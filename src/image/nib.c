/*
 * nib.c
 *
 * Apple2 nibble images:
 *
 * .nib: 35 tracks of 6656 GCR bytes each, as the Disk II controller reads
 * them. Sync bytes have lost their two trailing zero bits in this format, so
 * the host's controller aligns itself on the data instead; it keeps the
 * alignment through the gaps, which are all ones.
 *
 * .nic, the format of the SDISK// emulator: 35 tracks of 16 blocks of 512
 * bytes, the first 416 bytes of each block one sector as a ready bit stream,
 * sync bits included, and the rest zero padding. A track streams as the 16
 * sectors' bit streams back to back: the same 6656 bytes as a .nib track.
 *
 * Both stream their bytes as bit cells, 4us each. A write is decoded back
 * into nibbles the way the controller does it, a nibble being eight bits from
 * a 1 bit on, and the nibbles go into the track once the write is complete:
 * into a .nib track where the write began; into the fixed places of a .nic
 * block, each address or data field found to the sector it belongs to.
 *
 * After the .nib handler of Oleg Odintsov's Gotek firmware 307 for the Agat,
 * the gotek-sa390 project of https://svn.code.sf.net/p/agat-hardware/code
 *
 * Written & released by Mike Shevchenko <mike.shevchenko.kz@gmail.com>
 *
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#define TRK_LEN 6656 /* bytes streamed per track, in both formats */
#define MAX_TRKS 40

#define NIC_BLK_LEN 512
#define NIC_BLK_DATA 416
#define NIC_BLKS_PER_TRK (TRK_LEN / NIC_BLK_DATA)
#define NIC_TRK_LEN (NIC_BLKS_PER_TRK * NIC_BLK_LEN)
/* Where the fields are in a block: D5 AA 96, the four 4-and-4 pairs and
 * DE AA EB; then D5 AA AD, 342 data nibbles, the checksum and DE AA EB. */
#define NIC_ADDR_OFF 34
#define NIC_ADDR_LEN 14
#define NIC_DATA_OFF 53
#define NIC_DATA_LEN 349

/* The staging buffer holds a block being modified, then the nibbles of the
 * write in progress, a track's worth: the volume cache has the rest. */
#define STAGE_BLK 0
#define STAGE_NIB NIC_BLK_LEN
#define STAGE_LEN (8*1024)
#define STAGE_NIB_MAX (STAGE_LEN - STAGE_NIB)

static void nib_seek_track(struct image *im, uint16_t track);

static bool_t nib_open(struct image *im)
{
    unsigned int trk_len = im->nib.nic ? NIC_TRK_LEN : TRK_LEN;
    unsigned int nr_trks = f_size(&im->fp) / trk_len;

    if ((f_size(&im->fp) % trk_len) || (nr_trks < 35) || (nr_trks > MAX_TRKS))
        return FALSE;

    im->nr_cyls = nr_trks;
    im->step = 2; /* the host steps in half tracks */
    im->nr_sides = 1;
    im->write_bc_ticks = sampleclk_us(4);
    im->ticks_per_cell = im->write_bc_ticks * 16;
    im->sync = SYNC_none;
    im->tracklen_bc = TRK_LEN * 8;
    im->tracklen_ticks = im->tracklen_bc * im->ticks_per_cell;
    im->stk_per_rev = stk_sampleclk(im->tracklen_ticks / 16);

    ASSERT(STAGE_LEN <= im->bufs.read_data.len);
    volume_cache_init(im->bufs.read_data.p + STAGE_LEN,
                      im->bufs.read_data.p + im->bufs.read_data.len);
    if (im->bufs.read_data.len < (64*1024))
        volume_cache_metadata_only(&im->fp);

    nib_seek_track(im, 0);

    return TRUE;
}

static bool_t nic_open(struct image *im)
{
    im->nib.nic = TRUE;
    return nib_open(im);
}

static void nib_seek_track(struct image *im, uint16_t track)
{
    im->nib.trk_off = (uint32_t)track * (im->nib.nic ? NIC_TRK_LEN : TRK_LEN);
    im->cur_track = track;
}

static void nib_setup_track(
    struct image *im, uint16_t track, uint32_t *start_pos)
{
    struct image_buf *rd = &im->bufs.read_data;
    struct image_buf *bc = &im->bufs.read_bc;
    uint32_t start_ticks;
    unsigned int blk = im->nib.nic ? NIC_BLK_DATA : 256;

    /* Half-track steps, one side. */
    track >>= 2;
    if (track != im->cur_track)
        nib_seek_track(im, track);

    start_ticks = start_pos ? *start_pos : get_write(im, im->wr_cons)->start;

    im->cur_ticks = start_ticks * 16;
    im->cur_bc = udiv64((uint64_t)im->cur_ticks * im->tracklen_bc,
                        im->tracklen_ticks);
    if ((im->cur_ticks >= im->tracklen_ticks) ||
        (im->cur_bc >= im->tracklen_bc)) {
        im->cur_ticks = 0;
        im->cur_bc = 0;
    }
    im->ticks_since_flux = 0;

    rd->prod = rd->cons = 0;
    bc->prod = bc->cons = 0;

    if (start_pos) {
        /* Read mode: the ring starts at the block holding the position,
         * and reading the first blocks moves trk_pos past them. */
        uint32_t pos = ((im->cur_bc/8) / blk) * blk;
        im->nib.trk_pos = pos;
        image_read_track(im);
        bc->cons = im->cur_bc - pos * 8;
    } else {
        /* Write mode: the nibbles are collected first. */
        im->nib.write.start = im->cur_bc;
        im->nib.write.nr = 0;
        im->nib.write.acc = im->nib.write.nbits = 0;
        im->nib.write.lost = FALSE;
    }
}

/* Copies @n bytes into the bit-cell ring at byte @pos, around its end. */
static void bc_copy(struct image_buf *bc, uint32_t pos, const uint8_t *src,
                    unsigned int n)
{
    uint8_t *bc_b = bc->p;
    uint32_t mask = bc->len - 1, off = pos & mask;
    unsigned int first = min_t(unsigned int, n, bc->len - off);

    memcpy(&bc_b[off], src, first);
    memcpy(bc_b, src + first, n - first);
}

static bool_t nib_read_track(struct image *im)
{
    struct image_buf *rd = &im->bufs.read_data;
    struct image_buf *bc = &im->bufs.read_bc;
    uint8_t *buf = rd->p;
    uint32_t bc_len, bc_space, bc_p, bc_c;
    unsigned int nr, blk_data, blk_file;

    if (im->nib.nic) {
        blk_data = NIC_BLK_DATA;
        blk_file = NIC_BLK_LEN;
    } else {
        blk_data = blk_file = 256;
    }

    if (rd->prod == rd->cons) {
        nr = min_t(unsigned int, 8, (TRK_LEN - im->nib.trk_pos) / blk_data);
        F_lseek(&im->fp, im->nib.trk_off
                + (im->nib.trk_pos / blk_data) * blk_file);
        F_read(&im->fp, buf, nr * blk_file, NULL);
        rd->cons = 0;
        rd->prod = nr;
        im->nib.trk_pos += nr * blk_data;
        if (im->nib.trk_pos >= TRK_LEN)
            im->nib.trk_pos = 0;
    }

    /* Fill the raw-bitcell ring buffer. */
    bc_p = bc->prod / 8;
    bc_c = bc->cons / 8;
    bc_len = bc->len;
    bc_space = bc_len - (uint16_t)(bc_p - bc_c);

    nr = min_t(unsigned int, rd->prod - rd->cons, bc_space / blk_data);
    if (nr == 0)
        return FALSE;

    while (nr--) {
        bc_copy(bc, bc_p, &buf[rd->cons * blk_file], blk_data);
        rd->cons++;
        bc_p += blk_data;
    }

    barrier();
    bc->prod = bc_p * 8;

    return TRUE;
}

/* Puts the @nr nibbles of the write that began at stream byte @pos into a
 * .nib track, around its end. */
static void nib_put_nibbles(struct image *im, const uint8_t *nib,
                            unsigned int nr, uint32_t pos)
{
    unsigned int first;

    if (nr > TRK_LEN)
        nr = TRK_LEN;

    first = min_t(unsigned int, nr, TRK_LEN - pos);
    F_lseek(&im->fp, im->nib.trk_off + pos);
    F_write(&im->fp, nib, first, NULL);
    if (first != nr) {
        F_lseek(&im->fp, im->nib.trk_off);
        F_write(&im->fp, nib + first, nr - first, NULL);
    }
}

/* Puts the @nr nibbles of the write that began at stream byte @pos into the
 * .nic blocks they belong to: an address field names its sector, and a data
 * field follows its address field, or is the one the write began in. */
static void nic_put_nibbles(struct image *im, const uint8_t *nib,
                            unsigned int nr, uint32_t pos)
{
    uint8_t *blk = (uint8_t *)im->bufs.write_data.p + STAGE_BLK;
    unsigned int cur = pos / NIC_BLK_DATA, loaded = ~0u, i = 0;
    unsigned int off, len;
    bool_t dirty = FALSE;

    while (i + 3 <= nr) {
        if ((nib[i] != 0xd5) || (nib[i+1] != 0xaa)) {
            i++;
            continue;
        }
        if ((nib[i+2] == 0x96) && (i + NIC_ADDR_LEN <= nr)) {
            /* The sector is the third pair of the address field. */
            cur = ((nib[i+7] << 1) | 1) & nib[i+8] & (NIC_BLKS_PER_TRK-1);
            off = NIC_ADDR_OFF;
            len = NIC_ADDR_LEN;
        } else if ((nib[i+2] == 0xad) && (i + NIC_DATA_LEN <= nr)) {
            off = NIC_DATA_OFF;
            len = NIC_DATA_LEN;
        } else {
            i++;
            continue;
        }
        if (loaded != cur) {
            if (dirty) {
                F_lseek(&im->fp, im->nib.trk_off + loaded * NIC_BLK_LEN);
                F_write(&im->fp, blk, NIC_BLK_LEN, NULL);
            }
            F_lseek(&im->fp, im->nib.trk_off + cur * NIC_BLK_LEN);
            F_read(&im->fp, blk, NIC_BLK_LEN, NULL);
            loaded = cur;
            dirty = FALSE;
        }
        memcpy(blk + off, nib + i, len);
        dirty = TRUE;
        i += len;
    }

    if (dirty) {
        F_lseek(&im->fp, im->nib.trk_off + loaded * NIC_BLK_LEN);
        F_write(&im->fp, blk, NIC_BLK_LEN, NULL);
    }
}

static bool_t nib_write_track(struct image *im)
{
    struct write *write = get_write(im, im->wr_cons);
    struct image_buf *wr = &im->bufs.write_bc;
    uint8_t *buf = wr->p;
    uint8_t *nib = (uint8_t *)im->bufs.write_data.p + STAGE_NIB;
    unsigned int bufmask = wr->len - 1;
    uint32_t c = wr->cons, p = wr->prod & ~31;
    uint8_t acc = im->nib.write.acc, nbits = im->nib.write.nbits;
    bool_t flush;

    /* The bit cells up to the end of the write, once that is known. */
    barrier();
    flush = (im->wr_cons != im->wr_bc);
    if (flush)
        p = write->bc_end;

    /* Decode as the controller does: a nibble is eight bits from a 1 bit on,
     * and the zeros between nibbles are the sync bits. */
    for (; c != p; c++) {
        unsigned int bit = (buf[(c/8) & bufmask] >> (7 - (c&7))) & 1;
        if ((nbits == 0) && !bit)
            continue;
        acc = (acc << 1) | bit;
        if (++nbits < 8)
            continue;
        if (im->nib.write.nr < STAGE_NIB_MAX)
            nib[im->nib.write.nr++] = acc;
        else
            im->nib.write.lost = TRUE;
        acc = nbits = 0;
    }
    im->nib.write.acc = acc;
    im->nib.write.nbits = nbits;
    wr->cons = c;

    if (!flush)
        return FALSE;

    if (im->nib.write.lost)
        printk("*** Write overflow, %u kept\n", im->nib.write.nr);
    if (im->nib.nic)
        nic_put_nibbles(im, nib, im->nib.write.nr, im->nib.write.start / 8);
    else
        nib_put_nibbles(im, nib, im->nib.write.nr, im->nib.write.start / 8);

    return TRUE;
}

const struct image_handler nib_image_handler = {
    .open = nib_open,
    .setup_track = nib_setup_track,
    .read_track = nib_read_track,
    .rdata_flux = bc_rdata_flux,
    .write_track = nib_write_track,
};

const struct image_handler nic_image_handler = {
    .open = nic_open,
    .setup_track = nib_setup_track,
    .read_track = nib_read_track,
    .rdata_flux = bc_rdata_flux,
    .write_track = nib_write_track,
};

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
