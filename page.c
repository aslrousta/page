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

#include "page.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>

struct pf
{
  int fd;                  /* file descriptor */
  uint8_t hpage[PFPAGESZ]; /* file header page */
  uint8_t page[PFPAGESZ];  /* current page */
  int pageno;              /* current page number */
};

/* get the magic signature bytes */
#define _hmagic(pf) *((uint32_t *)pf->hpage)

/* get the total number of pages  */
#define _npages(pf) *((uint32_t *)pf->hpage + 1)

/* get the first page in the free pages list */
#define _hfree(pf) *((uint32_t *)pf->hpage + 2)

/* get the next free page in the free pages list */
#define _nextfree(pf) *((uint32_t *)pf->page)

static int _pread (struct pf *pf);
static int _pwrite (struct pf *pf);
static int _hread (struct pf *pf);
static int _hwrite (struct pf *pf);

struct pf *
pf_open (const char *filename)
{
  struct pf *pf;
  struct stat stat;

  pf = (struct pf *)malloc (sizeof (struct pf));
  if (!pf)
    return errno = ENOMEM, NULL;

  memset (pf, 0, sizeof (struct pf));

  pf->fd = open (filename, O_RDWR | O_CREAT);
  if (!pf->fd)
    goto fail;

  if (fstat (pf->fd, &stat))
    goto fail;

  if (!stat.st_size)
    /* file is not empty, load file header */
    {
      if (_hread (pf))
        goto fail;
    }
  else
    /* otherwise, initialize a new pagefile */
    {
      _hmagic (pf) = PFMAGIC;
      if (_hwrite (pf))
        goto fail;
    }

  return pf;

fail:
  pf_close (pf);
  return NULL;
}

void
pf_close (struct pf *pf)
{
  if (pf)
    {
      if (pf->fd)
        close (pf->fd);
      free (pf);
    }
}

void *
pf_alloc (struct pf *pf, int *pageno)
{
  if ((*pageno = _hfree (pf)))
    {
      if (!pf_read (pf, *pageno))
        return NULL;

      _hfree (pf) = _nextfree (pf);
    }
  else
    {
      pf->pageno = *pageno = (int)(_npages (pf));
      _npages (pf) = _npages (pf) + 1;
    }

  if (_hwrite (pf))
    return NULL;

  memset (pf->page, 0, PFPAGESZ);
  if (pf_write (pf))
    return NULL;

  return pf->page;
}

int
pf_free (struct pf *pf)
{
  if (!pf->pageno)
    return errno = EINVAL, -1;

  _nextfree (pf) = _hfree (pf);
  _hfree (pf) = (uint32_t)pf->pageno;

  if (_pwrite (pf) || _hwrite (pf))
    return -1;

  pf->pageno = 0;
  return 0;
}

void *
pf_read (struct pf *pf, int pageno)
{
  if (!pf || pageno <= 0)
    return errno = EINVAL, NULL;

  pf->pageno = pageno;
  if (_pread (pf))
    return NULL;

  return pf->page;
}

int
pf_write (struct pf *pf)
{
  if (!pf || !pf->pageno)
    return errno = EINVAL, -1;

  return _pwrite (pf);
}

uint64_t
pfm_get_u64 (struct pf *pf, int off)
{
  uint8_t *p = pf->hpage + PFHDRSZ + off;

  if (p + sizeof (uint64_t) > pf->hpage + PFPAGESZ)
    return errno = EINVAL, UINT64_MAX;

  return *((uint64_t *)p);
}

uint32_t
pfm_get_u32 (struct pf *pf, int off)
{
  uint8_t *p = pf->hpage + PFHDRSZ + off;

  if (p + sizeof (uint32_t) > pf->hpage + PFPAGESZ)
    return errno = EINVAL, UINT32_MAX;

  return *((uint32_t *)p);
}

uint16_t
pfm_get_u16 (struct pf *pf, int off)
{
  uint8_t *p = pf->hpage + PFHDRSZ + off;

  if (p + sizeof (uint16_t) > pf->hpage + PFPAGESZ)
    return errno = EINVAL, UINT16_MAX;

  return *((uint16_t *)p);
}

uint8_t
pfm_get_u8 (struct pf *pf, int off)
{
  uint8_t *p = pf->hpage + PFHDRSZ + off;

  if (p + sizeof (uint8_t) > pf->hpage + PFPAGESZ)
    return errno = EINVAL, UINT8_MAX;

  return *p;
}

int
pfm_set_u64 (struct pf *pf, int off, uint64_t val)
{
  uint8_t *p = pf->hpage + PFHDRSZ + off;

  if (p + sizeof (uint64_t) > pf->hpage + PFPAGESZ)
    return errno = EINVAL, -1;

  *((uint64_t *)p) = val;
  return _hwrite (pf);
}

int
pfm_set_u32 (struct pf *pf, int off, uint32_t val)
{
  uint8_t *p = pf->hpage + PFHDRSZ + off;

  if (p + sizeof (uint32_t) > pf->hpage + PFPAGESZ)
    return errno = EINVAL, -1;

  *((uint32_t *)p) = val;
  return _hwrite (pf);
}

int
pfm_set_u16 (struct pf *pf, int off, uint16_t val)
{
  uint8_t *p = pf->hpage + PFHDRSZ + off;

  if (p + sizeof (uint16_t) > pf->hpage + PFPAGESZ)
    return errno = EINVAL, -1;

  *((uint16_t *)p) = val;
  return _hwrite (pf);
}

int
pfm_set_u8 (struct pf *pf, int off, uint8_t val)
{
  uint8_t *p = pf->hpage + PFHDRSZ + off;

  if (p + sizeof (uint8_t) > p + PFPAGESZ)
    return errno = EINVAL, -1;

  *p = val;
  return _hwrite (pf);
}

int
_pread (struct pf *pf)
{
  if (lseek (pf->fd, pf->pageno * PFPAGESZ, SEEK_SET) < 0
      || read (pf->fd, pf->page, PFPAGESZ) < 0)
    return -1;

  return 0;
}

int
_pwrite (struct pf *pf)
{
  if (lseek (pf->fd, pf->pageno * PFPAGESZ, SEEK_SET) < 0
      || write (pf->fd, pf->page, PFPAGESZ) < 0)
    return -1;

  return 0;
}

int
_hread (struct pf *pf)
{
  if (lseek (pf->fd, 0, SEEK_SET) < 0
      || read (pf->fd, pf->hpage, PFPAGESZ) < 0)
    return -1;

  if (_hmagic (pf) != PFMAGIC)
    return errno = EFTYPE, -1;

  return 0;
}

int
_hwrite (struct pf *pf)
{
  if (lseek (pf->fd, 0, SEEK_SET) < 0
      || write (pf->fd, pf->hpage, PFPAGESZ) < 0)
    return -1;

  return 0;
}
