#ifndef _MEMMGR_DUMB_H_
#define _MEMMGR_DUMB_H_ 1

struct memmgr_dumb
{
    page_directory_t *page_directory;
    uintptr_t next_free_frame;
    uintptr_t next_free_page;
    uintptr_t allocated_frames;
};
typedef struct memmgr_dumb memmgr_dumb_t;

void dumb_init(memmgr_dumb_t *memmgr_dumb, page_directory_t *page_directory);
void *dumb_alloc(memmgr_dumb_t *memmgr_dumb, uintptr_t size);
#endif
