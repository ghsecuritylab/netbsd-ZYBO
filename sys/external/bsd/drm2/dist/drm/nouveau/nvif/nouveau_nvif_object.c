/*	$NetBSD$	*/

/*
 * Copyright 2014 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs <bskeggs@redhat.com>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <nvif/object.h>
#include <nvif/client.h>
#include <nvif/driver.h>
#include <nvif/ioctl.h>

int
nvif_object_ioctl(struct nvif_object *object, void *data, u32 size, void **hack)
{
	struct nvif_client *client = object->client;
	union {
		struct nvif_ioctl_v0 v0;
	} *args = data;

	if (size >= sizeof(*args) && args->v0.version == 0) {
		if (object != &client->object)
			args->v0.object = nvif_handle(object);
		else
			args->v0.object = 0;
		args->v0.owner = NVIF_IOCTL_V0_OWNER_ANY;
	} else
		return -ENOSYS;

	return client->driver->ioctl(client->object.priv, client->super,
				     data, size, hack);
}

void
nvif_object_sclass_put(struct nvif_sclass **psclass)
{
	kfree(*psclass);
	*psclass = NULL;
}

int
nvif_object_sclass_get(struct nvif_object *object, struct nvif_sclass **psclass)
{
	struct {
		struct nvif_ioctl_v0 ioctl;
		struct nvif_ioctl_sclass_v0 sclass;
	} *args = NULL;
	int ret, cnt = 0, i;
	u32 size;

	while (1) {
		size = sizeof(*args) + cnt * sizeof(args->sclass.oclass[0]);
		if (!(args = kmalloc(size, GFP_KERNEL)))
			return -ENOMEM;
		args->ioctl.version = 0;
		args->ioctl.type = NVIF_IOCTL_V0_SCLASS;
		args->sclass.version = 0;
		args->sclass.count = cnt;

		ret = nvif_object_ioctl(object, args, size, NULL);
		if (ret == 0 && args->sclass.count <= cnt)
			break;
		cnt = args->sclass.count;
		kfree(args);
		if (ret != 0)
			return ret;
	}

	*psclass = kzalloc(sizeof(**psclass) * args->sclass.count, GFP_KERNEL);
	if (*psclass) {
		for (i = 0; i < args->sclass.count; i++) {
			(*psclass)[i].oclass = args->sclass.oclass[i].oclass;
			(*psclass)[i].minver = args->sclass.oclass[i].minver;
			(*psclass)[i].maxver = args->sclass.oclass[i].maxver;
		}
		ret = args->sclass.count;
	} else {
		ret = -ENOMEM;
	}

	kfree(args);
	return ret;
}

u32
nvif_object_rd(struct nvif_object *object, int size, u64 addr)
{
	struct {
		struct nvif_ioctl_v0 ioctl;
		struct nvif_ioctl_rd_v0 rd;
	} args = {
		.ioctl.type = NVIF_IOCTL_V0_RD,
		.rd.size = size,
		.rd.addr = addr,
	};
	int ret = nvif_object_ioctl(object, &args, sizeof(args), NULL);
	if (ret) {
		/*XXX: warn? */
		return 0;
	}
	return args.rd.data;
}

void
nvif_object_wr(struct nvif_object *object, int size, u64 addr, u32 data)
{
	struct {
		struct nvif_ioctl_v0 ioctl;
		struct nvif_ioctl_wr_v0 wr;
	} args = {
		.ioctl.type = NVIF_IOCTL_V0_WR,
		.wr.size = size,
		.wr.addr = addr,
		.wr.data = data,
	};
	int ret = nvif_object_ioctl(object, &args, sizeof(args), NULL);
	if (ret) {
		/*XXX: warn? */
	}
}

int
nvif_object_mthd(struct nvif_object *object, u32 mthd, void *data, u32 size)
{
	struct {
		struct nvif_ioctl_v0 ioctl;
		struct nvif_ioctl_mthd_v0 mthd;
	} *args;
	u8 stack[128];
	int ret;

	if (sizeof(*args) + size > sizeof(stack)) {
		if (!(args = kmalloc(sizeof(*args) + size, GFP_KERNEL)))
			return -ENOMEM;
	} else {
		args = (void *)stack;
	}
	args->ioctl.version = 0;
	args->ioctl.type = NVIF_IOCTL_V0_MTHD;
	args->mthd.version = 0;
	args->mthd.method = mthd;

	memcpy(args->mthd.data, data, size);
	ret = nvif_object_ioctl(object, args, sizeof(*args) + size, NULL);
	memcpy(data, args->mthd.data, size);
	if (args != (void *)stack)
		kfree(args);
	return ret;
}

void
nvif_object_unmap(struct nvif_object *object)
{
	if (object->map.size) {
		struct nvif_client *client = object->client;
		struct {
			struct nvif_ioctl_v0 ioctl;
			struct nvif_ioctl_unmap unmap;
		} args = {
			.ioctl.type = NVIF_IOCTL_V0_UNMAP,
		};

		if (object->map.ptr) {
			client->driver->unmap(client, object->map.tag,
						      object->map.handle,
						      object->map.addr,
						      object->map.ptr,
						      object->map.size);
			object->map.ptr = NULL;
		}

		nvif_object_ioctl(object, &args, sizeof(args), NULL);
		object->map.size = 0;
	}
}

int
nvif_object_map(struct nvif_object *object)
{
	struct nvif_client *client = object->client;
	struct {
		struct nvif_ioctl_v0 ioctl;
#ifdef __NetBSD__
		struct nvif_ioctl_map_netbsd_v0 map;
#else
		struct nvif_ioctl_map_v0 map;
#endif
	} args = {
#ifdef __NetBSD__
		.ioctl.type = NVIF_IOCTL_V0_MAP_NETBSD,
#else
		.ioctl.type = NVIF_IOCTL_V0_MAP,
#endif
	};
	int ret = nvif_object_ioctl(object, &args, sizeof(args), NULL);
	if (ret == 0) {
		object->map.size = args.map.length;
#ifdef __NetBSD__
		object->map.tag = args.map.tag;
		object->map.addr = args.map.handle;
		ret = client->driver->map(client, args.map.tag,
		    args.map.handle, object->map.size, &object->map.handle,
		    &object->map.ptr);
		if (ret) {
			nvif_object_unmap(object);
			return -ENOMEM;
		}
#else
		object->map.ptr = client->driver->map(client, args.map.handle,
						      object->map.size);
		if (ret = -ENOMEM, object->map.ptr)
			return 0;
		nvif_object_unmap(object);
#endif
	}
	return ret;
}

void
nvif_object_fini(struct nvif_object *object)
{
	struct {
		struct nvif_ioctl_v0 ioctl;
		struct nvif_ioctl_del del;
	} args = {
		.ioctl.type = NVIF_IOCTL_V0_DEL,
	};

	if (!object->client)
		return;

	nvif_object_unmap(object);
	nvif_object_ioctl(object, &args, sizeof(args), NULL);
	object->client = NULL;
}

int
nvif_object_init(struct nvif_object *parent, u32 handle, s32 oclass,
		 void *data, u32 size, struct nvif_object *object)
{
	struct {
		struct nvif_ioctl_v0 ioctl;
		struct nvif_ioctl_new_v0 new;
	} *args;
	int ret = 0;

	object->client = NULL;
	object->handle = handle;
	object->oclass = oclass;
	object->map.ptr = NULL;
	object->map.size = 0;

	if (parent) {
		if (!(args = kmalloc(sizeof(*args) + size, GFP_KERNEL))) {
			nvif_object_fini(object);
			return -ENOMEM;
		}

		args->ioctl.version = 0;
		args->ioctl.type = NVIF_IOCTL_V0_NEW;
		args->new.version = 0;
		args->new.route = parent->client->route;
		args->new.token = nvif_handle(object);
		args->new.object = nvif_handle(object);
		args->new.handle = handle;
		args->new.oclass = oclass;

		memcpy(args->new.data, data, size);
		ret = nvif_object_ioctl(parent, args, sizeof(*args) + size,
					&object->priv);
		memcpy(data, args->new.data, size);
		kfree(args);
		if (ret == 0)
			object->client = parent->client;
	}

	if (ret)
		nvif_object_fini(object);
	return ret;
}
