#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <res-proto.h>
#include "dbus-msg.h"
#include "dbus-proto.h"
#include "internal-msg.h"
#include "internal-proto.h"


static uint32_t  internal_reqno;





int resproto_set_handler(resconn_t           *rp,
                         resmsg_type_t        type,
                         resproto_handler_t   handler)
{
    if (type < 0 || type >= RESMSG_MAX || !rp->any.valid[type] || !handler)
        return FALSE;

   rp->any.handler[type] = handler;

    return TRUE;
}

int resproto_send_message(resset_t          *rset,
                          resmsg_t          *resmsg,
                          resproto_status_t  status)
{
    resconn_t       *rp   = rset->resconn;
    resproto_role_t  role = rp->any.role;
    resmsg_type_t    type = resmsg->type;
    int              success;

    if (rset->state != RESPROTO_RSET_STATE_CONNECTED ||
        type == RESMSG_REGISTER || type == RESMSG_UNREGISTER)
        success = FALSE;
    else
        success = rp->any.send(rset, resmsg, status);

    return success;
}

int resproto_reply_message(resset_t   *rset,
                           resmsg_t   *resmsg,
                           void       *protodata,
                           int32_t     errcod,
                           const char *errmsg)
{
    resconn_t *rp = rset->resconn;
    resmsg_t   reply;
    int        success;

    if (!rset || !resmsg)
        success = FALSE;
    else {
        if (protodata == NULL)
            success = TRUE;
        else {
            memset(&reply, 0, sizeof(reply));
            reply.status.type   = RESMSG_STATUS;
            reply.status.id     = rset->id;
            reply.status.reqno  = resmsg->any.reqno;
            reply.status.errcod = errcod;
            reply.status.errmsg = errmsg;
            
            success = rp->any.error(rset, &reply, protodata);
        }
    }

    return success;
}



/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
