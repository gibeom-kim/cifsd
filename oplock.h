/*
 *   fs/cifsd/oplock.h
 *
 *   Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *   Copyright (C) 2016 Namjae Jeon <namjae.jeon@protocolfreedom.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef __CIFSD_OPLOCK_H
#define __CIFSD_OPLOCK_H

#define OPLOCK_WAIT_TIME	(35*HZ)

/* SMB Oplock levels */
#define OPLOCK_NONE      0
#define OPLOCK_EXCLUSIVE 1
#define OPLOCK_BATCH     2
#define OPLOCK_READ      3  /* level 2 oplock */

/* SMB2 Oplock levels */
#define SMB2_OPLOCK_LEVEL_NONE          0x00
#define SMB2_OPLOCK_LEVEL_II            0x01
#define SMB2_OPLOCK_LEVEL_EXCLUSIVE     0x08
#define SMB2_OPLOCK_LEVEL_BATCH         0x09
#define SMB2_OPLOCK_LEVEL_LEASE         0xFF

/* Oplock states */
#define OPLOCK_STATE_NONE	0x00
#define OPLOCK_ACK_WAIT		0x01
#define OPLOCK_CLOSING		0x02

#define OPLOCK_WRITE_TO_READ		0x01
#define OPLOCK_READ_HANDLE_TO_READ	0x02
#define OPLOCK_WRITE_TO_NONE		0x04
#define OPLOCK_READ_TO_NONE		0x08

#define SMB2_LEASE_KEY_SIZE		16

extern struct mutex lease_list_lock;

struct lease_ctx_info {
	__u8	lease_key[SMB2_LEASE_KEY_SIZE];
	__le32	req_state;
	__le32	flags;
	__le64	duration;
	int dlease;
};

struct lease {
	__u8	lease_key[SMB2_LEASE_KEY_SIZE];
	__le32	state;
	__le32	new_state;
	__le32	flags;
	__le64	duration;
};

struct oplock_info {
	struct connection	*conn;
	struct cifsd_sess	*sess;
	struct smb_work		*work;
	bool			is_smb2;
	struct cifsd_file	*o_fp;
	int                     level;
	int                     op_state;
	uint64_t                fid;
	__u16                   Tid;
	atomic_t		breaking_cnt;
	bool			is_lease;
	struct lease		*o_lease;
	struct list_head        interim_list;
	struct list_head        op_entry;
	struct list_head        lease_entry;
	wait_queue_head_t oplock_q; /* Other server threads */
	wait_queue_head_t oplock_brk; /* oplock breaking wait */
	wait_queue_head_t	op_end_wq;
	bool			open_trunc:1;	/* truncate on open */
};

struct lease_table {
	char client_guid[SMB2_CLIENT_GUID_SIZE];
	struct list_head lease_list;
	struct list_head l_entry;
};

struct lease_break_info {
	int curr_state;
	int new_state;
	char lease_key[SMB2_LEASE_KEY_SIZE];
};

struct oplock_break_info {
	int level;
	int open_trunc;
	int fid;
};

extern int smb_grant_oplock(struct smb_work *work, int req_op_level,
		uint64_t id, struct cifsd_file *fp, __u16 Tid,
		struct lease_ctx_info *lctx);
extern void smb1_send_oplock_break_notification(struct work_struct *work);
#ifdef CONFIG_CIFS_SMB2_SERVER
extern void smb2_send_oplock_break_notification(struct work_struct *work);
#endif
extern void smb_break_all_levII_oplock(struct connection *conn,
	struct cifsd_file *fp, int is_trunc);

int opinfo_write_to_read(struct oplock_info *opinfo);
int opinfo_read_handle_to_read(struct oplock_info *opinfo);
int opinfo_write_to_none(struct oplock_info *opinfo);
int opinfo_read_to_none(struct oplock_info *opinfo);
void close_id_del_oplock(struct cifsd_file *fp);
void smb_break_all_oplock(struct smb_work *work, struct cifsd_file *fp);

#ifdef CONFIG_CIFS_SMB2_SERVER
/* Lease related functions */
void create_lease_buf(u8 *rbuf, struct lease *lease);
struct lease_ctx_info *parse_lease_state(void *open_req);
__u8 smb2_map_lease_to_oplock(__le32 lease_state);
int lease_read_to_write(struct oplock_info *opinfo);

/* Durable related functions */
void create_durable_rsp_buf(char *buf);
void create_durable_v2_rsp_buf(char *cc, struct cifsd_file *fp);
void create_mxac_rsp_buf(char *cc, int maximal_access);
void create_disk_id_rsp_buf(char *cc, __u64 file_id, __u64 vol_id);
struct create_context *smb2_find_context_vals(void *open_req, char *str);
int cifsd_durable_verify_and_del_oplock(struct cifsd_sess *curr_sess,
					  struct cifsd_sess *prev_sess,
					  int fid, struct file **filp,
					  uint64_t sess_id);
struct oplock_info *lookup_lease_in_table(struct connection *conn,
	char *lease_key);
int find_same_lease_key(struct cifsd_sess *sess, struct cifsd_mfile *mfp,
	struct lease_ctx_info *lctx);
void destroy_lease_table(struct connection *conn);
int smb2_check_durable_oplock(struct cifsd_file *fp,
	struct lease_ctx_info *lctx, char *name, int version);
#endif

#endif /* __CIFSD_OPLOCK_H */
