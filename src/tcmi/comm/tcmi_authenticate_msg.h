/**
 * @file tcmi_authenticate_msg.h - authenticate message
 */

#ifndef TCMI_AUTHENTICATE_MSG_H
#define TCMI_AUTHENTICATE_MSG_H

#include "tcmi_msg.h"
#include <arch/arch_ids.h>

/** @defgroup tcmi_authenticate_msg_class tcmi_authenticate_msg class
 *
 * @ingroup tcmi_msg_class
 *
 * This message is send by PEN when it wants authenticate to CCN.
 * 
 * @{
 */

/** Authentication message structure */
struct tcmi_authenticate_msg {
	/** parent class instance. */
	struct tcmi_msg super;
	/** Id of the requestor PEN */
	u_int32_t pen_id;
	/** Architecture of PEN */
	enum arch_ids pen_arch;

	/** Authentication data. Opaque to the kernel part, this is purely user space responsibilithy to provide and parse those. */
	char *auth_data;
	/** Size of authentication data */
	u_int32_t size;
};




/** \<\<public\>\> Authentication message consutructor for receiving. */
extern struct tcmi_msg* tcmi_authenticate_msg_new_rx(u_int32_t msg_id);

/** \<\<public\>\> Authentication message constructor for transferring. */
extern struct tcmi_msg* tcmi_authenticate_msg_new_tx(struct tcmi_slotvec *transactions, 
						       u_int32_t pen_id, enum arch_ids pen_arch, char* auth_data, int size);


/** \<\<public\>\> Message descriptor for the factory class, there is no error
 * handling as this is the starting request */
#define TCMI_AUTHENTICATE_MSG_DSC TCMI_MSG_DSC(TCMI_AUTHENTICATE_MSG_ID, tcmi_authenticate_msg_new_rx, NULL)

/** Casts to the tcmi_authenticate_msg instance. */
#define TCMI_AUTHENTICATE_MSG(m) ((struct tcmi_authenticate_msg*)m)

/**
 * \<\<public\>\> PEN id getter
 * 
 * @param *self - this message instance
 * @return PEN id
 */
static inline u_int32_t tcmi_authenticate_msg_pen_id(struct tcmi_authenticate_msg *self)
{
	return self->pen_id;
}

/**
 * \<\<public\>\> PEN architecture getter
 * 
 * @param *self - this message instance
 * @return PEN architecture
 */
static inline enum arch_ids tcmi_authenticate_msg_arch(struct tcmi_authenticate_msg *self)
{
	return self->pen_arch;
}

/**
 * \<\<public\>\> PEN authentication data length getter
 * 
 * @param *self - this message instance
 * @return Authentication data length
 */
static inline u_int32_t tcmi_authenticate_msg_auth_data_size(struct tcmi_authenticate_msg *self)
{
	return self->size;
}

/**
 * \<\<public\>\> PEN authentication data getter
 * 
 * @param *self - this message instance
 * @return Authentication data
 */
static inline char* tcmi_authenticate_msg_auth_data(struct tcmi_authenticate_msg *self)
{
	return self->auth_data;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_AUTHENTICATE_MSG_PRIVATE

/** Receives the message via a specified connection. */
static int tcmi_authenticate_msg_recv(struct tcmi_msg *self, struct kkc_sock *sock);

/** Sends the message via a specified connection. */
static int tcmi_authenticate_msg_send(struct tcmi_msg *self, struct kkc_sock *sock);

/** Message operations that support polymorphism. */
static struct tcmi_msg_ops authenticate_msg_ops;

#endif


/**
 * @}
 */


#endif

