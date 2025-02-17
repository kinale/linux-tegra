// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (c) by Jaroslav Kysela <perex@perex.cz>
 *                   Takashi Iwai <tiwai@suse.de>
 * 
 *  Generic memory allocators
 */

#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/genalloc.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_X86
#include <asm/set_memory.h>
#endif
#include <sound/memalloc.h>
#include "memalloc_local.h"

static const struct snd_malloc_ops *snd_dma_get_ops(struct snd_dma_buffer *dmab);

/* a cast to gfp flag from the dev pointer; for CONTINUOUS and VMALLOC types */
static inline gfp_t snd_mem_get_gfp_flags(const struct snd_dma_buffer *dmab,
					  gfp_t default_gfp)
{
	if (!dmab->dev.dev)
		return default_gfp;
	else
		return (__force gfp_t)(unsigned long)dmab->dev.dev;
}

static void *__snd_dma_alloc_pages(struct snd_dma_buffer *dmab, size_t size)
{
	const struct snd_malloc_ops *ops = snd_dma_get_ops(dmab);

	if (WARN_ON_ONCE(!ops || !ops->alloc))
		return NULL;
	return ops->alloc(dmab, size);
}

/**
 * snd_dma_alloc_pages - allocate the buffer area according to the given type
 * @type: the DMA buffer type
 * @device: the device pointer
 * @size: the buffer size to allocate
 * @dmab: buffer allocation record to store the allocated data
 *
 * Calls the memory-allocator function for the corresponding
 * buffer type.
 *
 * Return: Zero if the buffer with the given size is allocated successfully,
 * otherwise a negative value on error.
 */
int snd_dma_alloc_pages(int type, struct device *device, size_t size,
			struct snd_dma_buffer *dmab)
{
	if (WARN_ON(!size))
		return -ENXIO;
	if (WARN_ON(!dmab))
		return -ENXIO;

	size = PAGE_ALIGN(size);
	dmab->dev.type = type;
	dmab->dev.dev = device;
	dmab->bytes = 0;
	dmab->addr = 0;
	dmab->private_data = NULL;
	dmab->area = __snd_dma_alloc_pages(dmab, size);
	if (!dmab->area)
		return -ENOMEM;
	dmab->bytes = size;
	return 0;
}
EXPORT_SYMBOL(snd_dma_alloc_pages);

/**
 * snd_dma_alloc_pages_fallback - allocate the buffer area according to the given type with fallback
 * @type: the DMA buffer type
 * @device: the device pointer
 * @size: the buffer size to allocate
 * @dmab: buffer allocation record to store the allocated data
 *
 * Calls the memory-allocator function for the corresponding
 * buffer type.  When no space is left, this function reduces the size and
 * tries to allocate again.  The size actually allocated is stored in
 * res_size argument.
 *
 * Return: Zero if the buffer with the given size is allocated successfully,
 * otherwise a negative value on error.
 */
int snd_dma_alloc_pages_fallback(int type, struct device *device, size_t size,
				 struct snd_dma_buffer *dmab)
{
	int err;

	while ((err = snd_dma_alloc_pages(type, device, size, dmab)) < 0) {
		if (err != -ENOMEM)
			return err;
		if (size <= PAGE_SIZE)
			return -ENOMEM;
		size >>= 1;
		size = PAGE_SIZE << get_order(size);
	}
	if (! dmab->area)
		return -ENOMEM;
	return 0;
}
EXPORT_SYMBOL(snd_dma_alloc_pages_fallback);

/**
 * snd_dma_free_pages - release the allocated buffer
 * @dmab: the buffer allocation record to release
 *
 * Releases the allocated buffer via snd_dma_alloc_pages().
 */
void snd_dma_free_pages(struct snd_dma_buffer *dmab)
{
	const struct snd_malloc_ops *ops = snd_dma_get_ops(dmab);

	if (ops && ops->free)
		ops->free(dmab);
}
EXPORT_SYMBOL(snd_dma_free_pages);

/* called by devres */
static void __snd_release_pages(struct device *dev, void *res)
{
	snd_dma_free_pages(res);
}

/**
 * snd_devm_alloc_pages - allocate the buffer and manage with devres
 * @dev: the device pointer
 * @type: the DMA buffer type
 * @size: the buffer size to allocate
 *
 * Allocate buffer pages depending on the given type and manage using devres.
 * The pages will be released automatically at the device removal.
 *
 * Unlike snd_dma_alloc_pages(), this function requires the real device pointer,
 * hence it can't work with SNDRV_DMA_TYPE_CONTINUOUS or
 * SNDRV_DMA_TYPE_VMALLOC type.
 *
 * The function returns the snd_dma_buffer object at success, or NULL if failed.
 */
struct snd_dma_buffer *
snd_devm_alloc_pages(struct device *dev, int type, size_t size)
{
	struct snd_dma_buffer *dmab;
	int err;

	if (WARN_ON(type == SNDRV_DMA_TYPE_CONTINUOUS ||
		    type == SNDRV_DMA_TYPE_VMALLOC))
		return NULL;

	dmab = devres_alloc(__snd_release_pages, sizeof(*dmab), GFP_KERNEL);
	if (!dmab)
		return NULL;

	err = snd_dma_alloc_pages(type, dev, size, dmab);
	if (err < 0) {
		devres_free(dmab);
		return NULL;
	}

	devres_add(dev, dmab);
	return dmab;
}
EXPORT_SYMBOL_GPL(snd_devm_alloc_pages);

/**
 * snd_dma_buffer_mmap - perform mmap of the given DMA buffer
 * @dmab: buffer allocation information
 * @area: VM area information
 */
int snd_dma_buffer_mmap(struct snd_dma_buffer *dmab,
			struct vm_area_struct *area)
{
	const struct snd_malloc_ops *ops = snd_dma_get_ops(dmab);

	if (ops && ops->mmap)
		return ops->mmap(dmab, area);
	else
		return -ENOENT;
}
EXPORT_SYMBOL(snd_dma_buffer_mmap);

/**
 * snd_sgbuf_get_addr - return the physical address at the corresponding offset
 * @dmab: buffer allocation information
 * @offset: offset in the ring buffer
 */
dma_addr_t snd_sgbuf_get_addr(struct snd_dma_buffer *dmab, size_t offset)
{
	const struct snd_malloc_ops *ops = snd_dma_get_ops(dmab);

	if (ops && ops->get_addr)
		return ops->get_addr(dmab, offset);
	else
		return dmab->addr + offset;
}
EXPORT_SYMBOL(snd_sgbuf_get_addr);

/**
 * snd_sgbuf_get_page - return the physical page at the corresponding offset
 * @dmab: buffer allocation information
 * @offset: offset in the ring buffer
 */
struct page *snd_sgbuf_get_page(struct snd_dma_buffer *dmab, size_t offset)
{
	const struct snd_malloc_ops *ops = snd_dma_get_ops(dmab);

	if (ops && ops->get_page)
		return ops->get_page(dmab, offset);
	else
		return virt_to_page(dmab->area + offset);
}
EXPORT_SYMBOL(snd_sgbuf_get_page);

/**
 * snd_sgbuf_get_chunk_size - compute the max chunk size with continuous pages
 *	on sg-buffer
 * @dmab: buffer allocation information
 * @ofs: offset in the ring buffer
 * @size: the requested size
 */
unsigned int snd_sgbuf_get_chunk_size(struct snd_dma_buffer *dmab,
				      unsigned int ofs, unsigned int size)
{
	const struct snd_malloc_ops *ops = snd_dma_get_ops(dmab);

	if (ops && ops->get_chunk_size)
		return ops->get_chunk_size(dmab, ofs, size);
	else
		return size;
}
EXPORT_SYMBOL(snd_sgbuf_get_chunk_size);

/*
 * Continuous pages allocator
 */
static void *snd_dma_continuous_alloc(struct snd_dma_buffer *dmab, size_t size)
{
	gfp_t gfp = snd_mem_get_gfp_flags(dmab, GFP_KERNEL);
	void *p = alloc_pages_exact(size, gfp);

	if (p)
		dmab->addr = page_to_phys(virt_to_page(p));
	return p;
}

static void snd_dma_continuous_free(struct snd_dma_buffer *dmab)
{
	free_pages_exact(dmab->area, dmab->bytes);
}

static int snd_dma_continuous_mmap(struct snd_dma_buffer *dmab,
				   struct vm_area_struct *area)
{
	return remap_pfn_range(area, area->vm_start,
			       dmab->addr >> PAGE_SHIFT,
			       area->vm_end - area->vm_start,
			       area->vm_page_prot);
}

static const struct snd_malloc_ops snd_dma_continuous_ops = {
	.alloc = snd_dma_continuous_alloc,
	.free = snd_dma_continuous_free,
	.mmap = snd_dma_continuous_mmap,
};

/*
 * VMALLOC allocator
 */
static void *snd_dma_vmalloc_alloc(struct snd_dma_buffer *dmab, size_t size)
{
	gfp_t gfp = snd_mem_get_gfp_flags(dmab, GFP_KERNEL | __GFP_HIGHMEM);

	return __vmalloc(size, gfp);
}

static void snd_dma_vmalloc_free(struct snd_dma_buffer *dmab)
{
	vfree(dmab->area);
}

static int snd_dma_vmalloc_mmap(struct snd_dma_buffer *dmab,
				struct vm_area_struct *area)
{
	return remap_vmalloc_range(area, dmab->area, 0);
}

static dma_addr_t snd_dma_vmalloc_get_addr(struct snd_dma_buffer *dmab,
					   size_t offset)
{
	return page_to_phys(vmalloc_to_page(dmab->area + offset)) +
		offset % PAGE_SIZE;
}

static struct page *snd_dma_vmalloc_get_page(struct snd_dma_buffer *dmab,
					     size_t offset)
{
	return vmalloc_to_page(dmab->area + offset);
}

static unsigned int
snd_dma_vmalloc_get_chunk_size(struct snd_dma_buffer *dmab,
			       unsigned int ofs, unsigned int size)
{
	ofs %= PAGE_SIZE;
	size += ofs;
	if (size > PAGE_SIZE)
		size = PAGE_SIZE;
	return size - ofs;
}

static const struct snd_malloc_ops snd_dma_vmalloc_ops = {
	.alloc = snd_dma_vmalloc_alloc,
	.free = snd_dma_vmalloc_free,
	.mmap = snd_dma_vmalloc_mmap,
	.get_addr = snd_dma_vmalloc_get_addr,
	.get_page = snd_dma_vmalloc_get_page,
	.get_chunk_size = snd_dma_vmalloc_get_chunk_size,
};

#ifdef CONFIG_HAS_DMA
/*
 * IRAM allocator
 */
#ifdef CONFIG_GENERIC_ALLOCATOR
static void *snd_dma_iram_alloc(struct snd_dma_buffer *dmab, size_t size)
{
	struct device *dev = dmab->dev.dev;
	struct gen_pool *pool;
	void *p;

	if (dev->of_node) {
		pool = of_gen_pool_get(dev->of_node, "iram", 0);
		/* Assign the pool into private_data field */
		dmab->private_data = pool;

		p = gen_pool_dma_alloc_align(pool, size, &dmab->addr, PAGE_SIZE);
		if (p)
			return p;
	}

	/* Internal memory might have limited size and no enough space,
	 * so if we fail to malloc, try to fetch memory traditionally.
	 */
	dmab->dev.type = SNDRV_DMA_TYPE_DEV;
	return __snd_dma_alloc_pages(dmab, size);
}

static void snd_dma_iram_free(struct snd_dma_buffer *dmab)
{
	struct gen_pool *pool = dmab->private_data;

	if (pool && dmab->area)
		gen_pool_free(pool, (unsigned long)dmab->area, dmab->bytes);
}

static int snd_dma_iram_mmap(struct snd_dma_buffer *dmab,
			     struct vm_area_struct *area)
{
	area->vm_page_prot = pgprot_writecombine(area->vm_page_prot);
	return remap_pfn_range(area, area->vm_start,
			       dmab->addr >> PAGE_SHIFT,
			       area->vm_end - area->vm_start,
			       area->vm_page_prot);
}

static const struct snd_malloc_ops snd_dma_iram_ops = {
	.alloc = snd_dma_iram_alloc,
	.free = snd_dma_iram_free,
	.mmap = snd_dma_iram_mmap,
};
#endif /* CONFIG_GENERIC_ALLOCATOR */

#define DEFAULT_GFP \
	(GFP_KERNEL | \
	 __GFP_COMP |    /* compound page lets parts be mapped */ \
	 __GFP_NORETRY | /* don't trigger OOM-killer */ \
	 __GFP_NOWARN)   /* no stack trace print - this call is non-critical */

/*
 * Coherent device pages allocator
 */
static void *snd_dma_dev_alloc(struct snd_dma_buffer *dmab, size_t size)
{
	void *p;

	p = dma_alloc_coherent(dmab->dev.dev, size, &dmab->addr, DEFAULT_GFP);
#ifdef CONFIG_X86
	if (p && dmab->dev.type == SNDRV_DMA_TYPE_DEV_WC)
		set_memory_wc((unsigned long)p, PAGE_ALIGN(size) >> PAGE_SHIFT);
#endif
	return p;
}

static void snd_dma_dev_free(struct snd_dma_buffer *dmab)
{
#ifdef CONFIG_X86
	if (dmab->dev.type == SNDRV_DMA_TYPE_DEV_WC)
		set_memory_wb((unsigned long)dmab->area,
			      PAGE_ALIGN(dmab->bytes) >> PAGE_SHIFT);
#endif
	dma_free_coherent(dmab->dev.dev, dmab->bytes, dmab->area, dmab->addr);
}

static int snd_dma_dev_mmap(struct snd_dma_buffer *dmab,
			    struct vm_area_struct *area)
{
#ifdef CONFIG_X86
	if (dmab->dev.type == SNDRV_DMA_TYPE_DEV_WC)
		area->vm_page_prot = pgprot_writecombine(area->vm_page_prot);
#endif
	return dma_mmap_coherent(dmab->dev.dev, area,
				 dmab->area, dmab->addr, dmab->bytes);
}

static const struct snd_malloc_ops snd_dma_dev_ops = {
	.alloc = snd_dma_dev_alloc,
	.free = snd_dma_dev_free,
	.mmap = snd_dma_dev_mmap,
};

/*
 * Write-combined pages
 */
#ifdef CONFIG_X86
/* On x86, share the same ops as the standard dev ops */
#define snd_dma_wc_ops	snd_dma_dev_ops
#else /* CONFIG_X86 */
static void *snd_dma_wc_alloc(struct snd_dma_buffer *dmab, size_t size)
{
	return dma_alloc_wc(dmab->dev.dev, size, &dmab->addr, DEFAULT_GFP);
}

static void snd_dma_wc_free(struct snd_dma_buffer *dmab)
{
	dma_free_wc(dmab->dev.dev, dmab->bytes, dmab->area, dmab->addr);
}

static int snd_dma_wc_mmap(struct snd_dma_buffer *dmab,
			   struct vm_area_struct *area)
{
	return dma_mmap_wc(dmab->dev.dev, area,
			   dmab->area, dmab->addr, dmab->bytes);
}

static const struct snd_malloc_ops snd_dma_wc_ops = {
	.alloc = snd_dma_wc_alloc,
	.free = snd_dma_wc_free,
	.mmap = snd_dma_wc_mmap,
};
#endif /* CONFIG_X86 */
#endif /* CONFIG_HAS_DMA */

/*
 * Entry points
 */
static const struct snd_malloc_ops *dma_ops[] = {
	[SNDRV_DMA_TYPE_CONTINUOUS] = &snd_dma_continuous_ops,
	[SNDRV_DMA_TYPE_VMALLOC] = &snd_dma_vmalloc_ops,
#ifdef CONFIG_HAS_DMA
	[SNDRV_DMA_TYPE_DEV] = &snd_dma_dev_ops,
	[SNDRV_DMA_TYPE_DEV_WC] = &snd_dma_wc_ops,
#ifdef CONFIG_GENERIC_ALLOCATOR
	[SNDRV_DMA_TYPE_DEV_IRAM] = &snd_dma_iram_ops,
#endif /* CONFIG_GENERIC_ALLOCATOR */
#endif /* CONFIG_HAS_DMA */
#ifdef CONFIG_SND_DMA_SGBUF
	[SNDRV_DMA_TYPE_DEV_SG] = &snd_dma_sg_ops,
	[SNDRV_DMA_TYPE_DEV_WC_SG] = &snd_dma_sg_ops,
#endif
};

static const struct snd_malloc_ops *snd_dma_get_ops(struct snd_dma_buffer *dmab)
{
	if (WARN_ON_ONCE(dmab->dev.type <= SNDRV_DMA_TYPE_UNKNOWN ||
			 dmab->dev.type >= ARRAY_SIZE(dma_ops)))
		return NULL;
	return dma_ops[dmab->dev.type];
}
