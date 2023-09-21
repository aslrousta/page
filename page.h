/*  Page -- Page-file at its simplest
    Copyright (C) 2023  Ali AslRousta <me@alirus.ir>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#if !defined(PAGE_H__INCLUDED)
#define PAGE_H__INCLUDED

#include <stdint.h>

#define PFMAGIC 0x70716580            /* file magic signature */
#define PFPAGESZ 4096                 /* page size in bytes (4 kb) */
#define PFHDRSZ 32                    /* file header size in bytes */
#define PFMETASZ (PFPAGESZ - PFHDRSZ) /* file metadata size in bytes */

struct pf;

extern struct pf *pf_open (const char *filename);
extern void pf_close (struct pf *pf);

extern void *pf_alloc (struct pf *pf, int *pageno);
extern int pf_free (struct pf *pf);
extern void *pf_read (struct pf *pf, int pageno);
extern int pf_write (struct pf *pf);

extern uint64_t pfm_get_u64 (struct pf *pf, int off);
extern uint32_t pfm_get_u32 (struct pf *pf, int off);
extern uint16_t pfm_get_u16 (struct pf *pf, int off);
extern uint8_t pfm_get_u8 (struct pf *pf, int off);
extern int pfm_set_u64 (struct pf *pf, int off, uint64_t val);
extern int pfm_set_u32 (struct pf *pf, int off, uint32_t val);
extern int pfm_set_u16 (struct pf *pf, int off, uint16_t val);
extern int pfm_set_u8 (struct pf *pf, int off, uint8_t val);

#endif /* PAGE_H__INCLUDED */
