/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "kgsl.h"
#include "kgsl_sharedmem.h"
#include "kgsl_snapshot.h"

#include "adreno.h"
#include "adreno_pm4types.h"
#include "a2xx_reg.h"
#include "a3xx_reg.h"

#define NUM_DWORDS_OF_RINGBUFFER_HISTORY 100


#define SNAPSHOT_OBJ_BUFSIZE 64

#define SNAPSHOT_OBJ_TYPE_IB 0

static int snapshot_frozen_objsize;

static struct kgsl_snapshot_obj {
	int type;
	uint32_t gpuaddr;
	uint32_t ptbase;
	void *ptr;
	int dwords;
} objbuf[SNAPSHOT_OBJ_BUFSIZE];

static int objbufptr;

static void push_object(struct kgsl_device *device, int type, uint32_t ptbase,
	uint32_t gpuaddr, int dwords)
{
	int index;
	void *ptr;


	for (index = 0; index < objbufptr; index++) {
		if (objbuf[index].gpuaddr == gpuaddr &&
			objbuf[index].ptbase == ptbase) {
				objbuf[index].dwords = dwords;
				return;
			}
	}

	if (objbufptr == SNAPSHOT_OBJ_BUFSIZE) {
		KGSL_DRV_ERR(device, "snapshot: too many snapshot objects\n");
		return;
	}

	ptr = adreno_convertaddr(device, ptbase, gpuaddr, dwords << 2);

	if (ptr == NULL) {
		KGSL_DRV_ERR(device,
			"snapshot: Can't find GPU address for %x\n", gpuaddr);
		return;
	}

	
	objbuf[objbufptr].type = type;
	objbuf[objbufptr].gpuaddr = gpuaddr;
	objbuf[objbufptr].ptbase = ptbase;
	objbuf[objbufptr].dwords = dwords;
	objbuf[objbufptr++].ptr = ptr;
}


static int find_object(int type, unsigned int gpuaddr, unsigned int ptbase)
{
	int index;

	for (index = 0; index < objbufptr; index++) {
		if (objbuf[index].gpuaddr == gpuaddr &&
			objbuf[index].ptbase == ptbase &&
			objbuf[index].type == type)
			return 1;
	}

	return 0;
}


static struct {
	unsigned int base;
	unsigned int size;
} vsc_pipe[8];


static unsigned int vsc_size_address;


static struct {
	unsigned int base;
	unsigned int stride;
} vbo[16];


static unsigned int vfd_index_max;


static unsigned int vfd_control_0;


static unsigned int sp_vs_pvt_mem_addr;
static unsigned int sp_fs_pvt_mem_addr;


static int load_state_unit_sizes[7][2] = {
	{ 2, 4 },
	{ 0, 1 },
	{ 2, 4 },
	{ 0, 1 },
	{ 8, 2 },
	{ 8, 2 },
	{ 8, 2 },
};

static int ib_parse_load_state(struct kgsl_device *device, unsigned int *pkt,
	unsigned int ptbase)
{
	unsigned int block, source, type;
	int ret = 0;


	if (type3_pkt_size(pkt[0]) < 2)
		return 0;


	block = (pkt[1] >> 19) & 0x07;
	source = (pkt[1] >> 16) & 0x07;
	type = pkt[2] & 0x03;

	if (source == 4) {
		int unitsize, ret;

		if (type == 0)
			unitsize = load_state_unit_sizes[block][0];
		else
			unitsize = load_state_unit_sizes[block][1];

		

		ret = kgsl_snapshot_get_object(device, ptbase,
				pkt[2] & 0xFFFFFFFC,
				(((pkt[1] >> 22) & 0x03FF) * unitsize) << 2,
				SNAPSHOT_GPU_OBJECT_SHADER);

		if (ret < 0)
			return -EINVAL;

		snapshot_frozen_objsize += ret;
	}

	return ret;
}


static int ib_parse_set_bin_data(struct kgsl_device *device, unsigned int *pkt,
	unsigned int ptbase)
{
	int ret;

	if (type3_pkt_size(pkt[0]) < 2)
		return 0;

	
	ret = kgsl_snapshot_get_object(device, ptbase, pkt[1], 0,
			SNAPSHOT_GPU_OBJECT_GENERIC);

	if (ret < 0)
		return -EINVAL;

	snapshot_frozen_objsize += ret;

	
	ret = kgsl_snapshot_get_object(device, ptbase, pkt[2], 32,
			SNAPSHOT_GPU_OBJECT_GENERIC);

	if (ret >= 0)
		snapshot_frozen_objsize += ret;

	return ret;
}

/*
 * This opcode writes to GPU memory - if the buffer is written to, there is a
 * good chance that it would be valuable to capture in the snapshot, so mark all
 * buffers that are written to as frozen
 */

static int ib_parse_mem_write(struct kgsl_device *device, unsigned int *pkt,
	unsigned int ptbase)
{
	int ret;

	if (type3_pkt_size(pkt[0]) < 1)
		return 0;

	/*
	 * The address is where the data in the rest of this packet is written
	 * to, but since that might be an offset into the larger buffer we need
	 * to get the whole thing. Pass a size of 0 kgsl_snapshot_get_object to
	 * capture the entire buffer.
	 */

	ret = kgsl_snapshot_get_object(device, ptbase, pkt[1] & 0xFFFFFFFC, 0,
		SNAPSHOT_GPU_OBJECT_GENERIC);

	if (ret >= 0)
		snapshot_frozen_objsize += ret;

	return ret;
}


static int ib_parse_draw_indx(struct kgsl_device *device, unsigned int *pkt,
	unsigned int ptbase)
{
	int ret = 0, i;

	if (type3_pkt_size(pkt[0]) < 3)
		return 0;

	

	if (type3_pkt_size(pkt[0]) > 3) {
		ret = kgsl_snapshot_get_object(device, ptbase, pkt[4], pkt[5],
			SNAPSHOT_GPU_OBJECT_GENERIC);
		if (ret < 0)
			return -EINVAL;

		snapshot_frozen_objsize += ret;
	}


	

	for (i = 0; i < ARRAY_SIZE(vsc_pipe); i++) {
		if (vsc_pipe[i].base != 0 && vsc_pipe[i].size != 0) {
			ret = kgsl_snapshot_get_object(device, ptbase,
				vsc_pipe[i].base, vsc_pipe[i].size,
				SNAPSHOT_GPU_OBJECT_GENERIC);
			if (ret < 0)
				return -EINVAL;

			snapshot_frozen_objsize += ret;
		}
	}

	

	if (vsc_size_address) {
		ret = kgsl_snapshot_get_object(device, ptbase,
				vsc_size_address, 32,
				SNAPSHOT_GPU_OBJECT_GENERIC);
		if (ret < 0)
			return -EINVAL;

		snapshot_frozen_objsize += ret;
	}

	
	if (sp_vs_pvt_mem_addr) {
		ret = kgsl_snapshot_get_object(device, ptbase,
				sp_vs_pvt_mem_addr, 8192,
				SNAPSHOT_GPU_OBJECT_GENERIC);
		if (ret < 0)
			return -EINVAL;

		snapshot_frozen_objsize += ret;
		sp_vs_pvt_mem_addr = 0;
	}

	if (sp_fs_pvt_mem_addr) {
		ret = kgsl_snapshot_get_object(device, ptbase,
				sp_fs_pvt_mem_addr, 8192,
				SNAPSHOT_GPU_OBJECT_GENERIC);
		if (ret < 0)
			return -EINVAL;

		snapshot_frozen_objsize += ret;
		sp_fs_pvt_mem_addr = 0;
	}

	

	
	for (i = 0; i < (vfd_control_0) >> 27; i++) {
		int size;


		if (vbo[i].base != 0) {
			size = vbo[i].stride * vfd_index_max;

			ret = kgsl_snapshot_get_object(device, ptbase,
				vbo[i].base,
				0, SNAPSHOT_GPU_OBJECT_GENERIC);
			if (ret < 0)
				return -EINVAL;

			snapshot_frozen_objsize += ret;
		}

		vbo[i].base = 0;
		vbo[i].stride = 0;
	}

	vfd_control_0 = 0;
	vfd_index_max = 0;

	return ret;
}


static int ib_parse_type3(struct kgsl_device *device, unsigned int *ptr,
	unsigned int ptbase)
{
	int opcode = cp_type3_opcode(*ptr);

	if (opcode == CP_LOAD_STATE)
		return ib_parse_load_state(device, ptr, ptbase);
	else if (opcode == CP_SET_BIN_DATA)
		return ib_parse_set_bin_data(device, ptr, ptbase);
	else if (opcode == CP_MEM_WRITE)
		return ib_parse_mem_write(device, ptr, ptbase);
	else if (opcode == CP_DRAW_INDX)
		return ib_parse_draw_indx(device, ptr, ptbase);

	return 0;
}

/*
 * Parse type0 packets found in the stream.  Some of the registers that are
 * written are clues for GPU buffers that we need to freeze.  Register writes
 * are considred valid when a draw initator is called, so just cache the values
 * here and freeze them when a CP_DRAW_INDX is seen.  This protects against
 * needlessly caching buffers that won't be used during a draw call
 */

static void ib_parse_type0(struct kgsl_device *device, unsigned int *ptr,
	unsigned int ptbase)
{
	int size = type0_pkt_size(*ptr);
	int offset = type0_pkt_offset(*ptr);
	int i;

	for (i = 0; i < size; i++, offset++) {

		

		if (offset >= A3XX_VSC_PIPE_DATA_ADDRESS_0 &&
			offset <= A3XX_VSC_PIPE_DATA_LENGTH_7) {
			int index = offset - A3XX_VSC_PIPE_DATA_ADDRESS_0;


			if ((index % 3) == 0)
				vsc_pipe[index / 3].base = ptr[i + 1];
			else if ((index % 3) == 1)
				vsc_pipe[index / 3].size = ptr[i + 1];
		} else if ((offset >= A3XX_VFD_FETCH_INSTR_0_0) &&
			(offset <= A3XX_VFD_FETCH_INSTR_1_F)) {
			int index = offset - A3XX_VFD_FETCH_INSTR_0_0;


			if ((index % 2) == 0)
				vbo[index >> 1].stride =
					(ptr[i + 1] >> 7) & 0x1FF;
			else
				vbo[index >> 1].base = ptr[i + 1];
		} else {

			switch (offset) {
			case A3XX_VFD_CONTROL_0:
				vfd_control_0 = ptr[i + 1];
				break;
			case A3XX_VFD_INDEX_MAX:
				vfd_index_max = ptr[i + 1];
				break;
			case A3XX_VSC_SIZE_ADDRESS:
				vsc_size_address = ptr[i + 1];
				break;
			case A3XX_SP_VS_PVT_MEM_ADDR_REG:
				sp_vs_pvt_mem_addr = ptr[i + 1];
				break;
			case A3XX_SP_FS_PVT_MEM_ADDR_REG:
				sp_fs_pvt_mem_addr = ptr[i + 1];
				break;
			}
		}
	}
}


static int ib_add_gpu_object(struct kgsl_device *device, unsigned int ptbase,
		unsigned int gpuaddr, unsigned int dwords)
{
	int i, ret, rem = dwords;
	unsigned int *src;


	if (kgsl_snapshot_have_object(device, ptbase, gpuaddr, dwords << 2))
		return 0;

	src = (unsigned int *) adreno_convertaddr(device, ptbase, gpuaddr,
		dwords << 2);

	if (src == NULL)
		return -EINVAL;

	for (i = 0; rem > 0; rem--, i++) {
		int pktsize;


		if (!pkt_is_type0(src[i]) && !pkt_is_type3(src[i]))
			break;

		pktsize = type3_pkt_size(src[i]);

		if (!pktsize || (pktsize + 1) > rem)
			break;

		if (pkt_is_type3(src[i])) {
			if (adreno_cmd_is_ib(src[i])) {
				unsigned int gpuaddr = src[i + 1];
				unsigned int size = src[i + 2];
				unsigned int ibbase;

				
				kgsl_regread(device, REG_CP_IB2_BASE, &ibbase);


				if (ibbase == gpuaddr)
					push_object(device,
						SNAPSHOT_OBJ_TYPE_IB, ptbase,
						gpuaddr, size);
				else {
					ret = ib_add_gpu_object(device,
						ptbase, gpuaddr, size);

					if (ret < 0)
						goto done;
				}
			} else {
				ret = ib_parse_type3(device, &src[i], ptbase);

				if (ret < 0)
					goto done;
			}
		} else if (pkt_is_type0(src[i])) {
			ib_parse_type0(device, &src[i], ptbase);
		}

		i += pktsize;
		rem -= pktsize;
	}

done:
	ret = kgsl_snapshot_get_object(device, ptbase, gpuaddr, dwords << 2,
		SNAPSHOT_GPU_OBJECT_IB);

	if (ret >= 0)
		snapshot_frozen_objsize += ret;

	return ret;
}

static int snapshot_istore(struct kgsl_device *device, void *snapshot,
	int remain, void *priv)
{
	struct kgsl_snapshot_istore *header = snapshot;
	unsigned int *data = snapshot + sizeof(*header);
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	int count, i;

	count = adreno_dev->istore_size * adreno_dev->instruction_size;

	if (remain < (count * 4) + sizeof(*header)) {
		KGSL_DRV_ERR(device,
			"snapshot: Not enough memory for the istore section");
		return 0;
	}

	header->count = adreno_dev->istore_size;

	for (i = 0; i < count; i++)
		kgsl_regread(device, ADRENO_ISTORE_START + i, &data[i]);

	return (count * 4) + sizeof(*header);
}

static int snapshot_rb(struct kgsl_device *device, void *snapshot,
	int remain, void *priv)
{
	struct kgsl_snapshot_rb *header = snapshot;
	unsigned int *data = snapshot + sizeof(*header);
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;
	unsigned int ptbase, rptr, *rbptr, ibbase;
	int index, size, i;
	int parse_ibs = 0, ib_parse_start;

	
	ptbase = kgsl_mmu_get_current_ptbase(&device->mmu);

	
	kgsl_regread(device, REG_CP_RB_RPTR, &rptr);

	
	kgsl_regread(device, REG_CP_IB1_BASE, &ibbase);


	index = rptr;
	rbptr = rb->buffer_desc.hostptr;

	do {
		index--;

		if (index < 0) {
			index = rb->sizedwords - 3;

			
			if (index < rb->wptr) {
				index = rb->wptr;
				break;
			}
		}

		if (adreno_cmd_is_ib(rbptr[index]) &&
			rbptr[index + 1] == ibbase)
			break;
	} while (index != rb->wptr);


	while (index != rb->wptr) {
		index--;

		if (index < 0) {
			index = rb->sizedwords - 2;


			if (index < rb->wptr) {
				index = rb->wptr;
				break;
			}
		}

		
		if ((rbptr[index] == cp_nop_packet(1)) &&
			(rbptr[index + 1] == KGSL_CONTEXT_TO_MEM_IDENTIFIER))
			break;
	}


	ib_parse_start = index;


	size = (rb->sizedwords << 2);

	if (remain < size + sizeof(*header)) {
		KGSL_DRV_ERR(device,
			"snapshot: Not enough memory for the rb section");
		return 0;
	}

	
	header->start = rb->wptr;
	header->end = rb->wptr;
	header->wptr = rb->wptr;
	header->rbsize = rb->sizedwords;
	header->count = rb->sizedwords;


	index = rb->wptr;
	for (i = 0; i < rb->sizedwords; i++) {
		*data = rbptr[index];


		if (parse_ibs == 0 && index == ib_parse_start)
			parse_ibs = 1;
		else if (index == rptr || adreno_rb_ctxtswitch(&rbptr[index]))
			parse_ibs = 0;

		if (parse_ibs && adreno_cmd_is_ib(rbptr[index])) {
			unsigned int ibaddr = rbptr[index + 1];
			unsigned int ibsize = rbptr[index + 2];


			struct kgsl_memdesc *memdesc =
				adreno_find_ctxtmem(device, ptbase, ibaddr,
					ibsize);

			
			if (NULL == memdesc)
				if (kgsl_gpuaddr_in_memdesc(
						&device->mmu.setstate_memory,
						ibaddr, ibsize))
					memdesc = &device->mmu.setstate_memory;

			if (ibaddr == ibbase || memdesc != NULL)
				push_object(device, SNAPSHOT_OBJ_TYPE_IB,
					ptbase, ibaddr, ibsize);
			else
				ib_add_gpu_object(device, ptbase, ibaddr,
					ibsize);
		}

		index = index + 1;

		if (index == rb->sizedwords)
			index = 0;

		data++;
	}

	
	return size + sizeof(*header);
}

static int snapshot_ib(struct kgsl_device *device, void *snapshot,
	int remain, void *priv)
{
	struct kgsl_snapshot_ib *header = snapshot;
	struct kgsl_snapshot_obj *obj = priv;
	unsigned int *src = obj->ptr;
	unsigned int *dst = snapshot + sizeof(*header);
	int i, ret;

	if (remain < (obj->dwords << 2) + sizeof(*header)) {
		KGSL_DRV_ERR(device,
			"snapshot: Not enough memory for the ib section");
		return 0;
	}

	
	header->gpuaddr = obj->gpuaddr;
	header->ptbase = obj->ptbase;
	header->size = obj->dwords;

	
	for (i = 0; i < obj->dwords; i++, src++, dst++) {
		*dst = *src;

		if (pkt_is_type3(*src)) {
			if ((obj->dwords - i) < type3_pkt_size(*src) + 1)
				continue;

			if (adreno_cmd_is_ib(*src))
				push_object(device, SNAPSHOT_OBJ_TYPE_IB,
					obj->ptbase, src[1], src[2]);
			else {
				ret = ib_parse_type3(device, src, obj->ptbase);

				
				if (ret < 0)
					break;
			}
		}
	}

	return (obj->dwords << 2) + sizeof(*header);
}

static void *dump_object(struct kgsl_device *device, int obj, void *snapshot,
	int *remain)
{
	switch (objbuf[obj].type) {
	case SNAPSHOT_OBJ_TYPE_IB:
		snapshot = kgsl_snapshot_add_section(device,
			KGSL_SNAPSHOT_SECTION_IB, snapshot, remain,
			snapshot_ib, &objbuf[obj]);
		break;
	default:
		KGSL_DRV_ERR(device,
			"snapshot: Invalid snapshot object type: %d\n",
			objbuf[obj].type);
		break;
	}

	return snapshot;
}


void *adreno_snapshot(struct kgsl_device *device, void *snapshot, int *remain,
		int hang)
{
	int i;
	uint32_t ptbase, ibbase, ibsize;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	
	objbufptr = 0;

	snapshot_frozen_objsize = 0;

	

	vfd_control_0 = 0;
	vfd_index_max = 0;
	vsc_size_address = 0;

	memset(vsc_pipe, 0, sizeof(vsc_pipe));
	memset(vbo, 0, sizeof(vbo));

	
	ptbase = kgsl_mmu_get_current_ptbase(&device->mmu);

	
	snapshot = kgsl_snapshot_add_section(device, KGSL_SNAPSHOT_SECTION_RB,
		snapshot, remain, snapshot_rb, NULL);


	kgsl_regread(device, REG_CP_IB1_BASE, &ibbase);
	kgsl_regread(device, REG_CP_IB1_BUFSZ, &ibsize);


	if (!find_object(SNAPSHOT_OBJ_TYPE_IB, ibbase, ptbase) && ibsize) {
		push_object(device, SNAPSHOT_OBJ_TYPE_IB, ptbase,
			ibbase, ibsize);
		KGSL_DRV_ERR(device, "CP_IB1_BASE not found in the ringbuffer. "
			"Dumping %x dwords of the buffer.\n", ibsize);
	}

	kgsl_regread(device, REG_CP_IB2_BASE, &ibbase);
	kgsl_regread(device, REG_CP_IB2_BUFSZ, &ibsize);


	if (!find_object(SNAPSHOT_OBJ_TYPE_IB, ibbase, ptbase) && ibsize) {
		push_object(device, SNAPSHOT_OBJ_TYPE_IB, ptbase,
			ibbase, ibsize);
	}

	for (i = 0; i < objbufptr; i++)
		snapshot = dump_object(device, i, snapshot, remain);


	if (hang) {
		snapshot = kgsl_snapshot_add_section(device,
			KGSL_SNAPSHOT_SECTION_ISTORE, snapshot, remain,
			snapshot_istore, NULL);
	}

	
	if (adreno_dev->gpudev->snapshot)
		snapshot = adreno_dev->gpudev->snapshot(adreno_dev, snapshot,
			remain, hang);

	if (snapshot_frozen_objsize)
		KGSL_DRV_ERR(device, "GPU snapshot froze %dKb of GPU buffers\n",
			snapshot_frozen_objsize / 1024);

	return snapshot;
}
