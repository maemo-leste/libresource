/*************************************************************************
This file is part of libresource

Copyright (C) 2010 Nokia Corporation.

This library is free software; you can redistribute
it and/or modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation
version 2.1 of the License.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
USA.
*************************************************************************/


#include <stdlib.h>
#include <string.h>

#include "res-msg.h"
#include "dbus-msg.h"

DBusMessage *resmsg_dbus_compose_message(const char *dest,
                                         const char *path,
                                         const char *interface,
                                         const char *method,
                                         resmsg_t   *resmsg)
{
    static char       *empty_str = "";

    DBusMessage       *dbusmsg = NULL;
    resmsg_record_t   *record;
    resmsg_possess_t  *possess;
    resmsg_notify_t   *notify;
    resmsg_audio_t    *audio;
    resmsg_video_t    *video;
    resmsg_property_t *property;
    int                success;

    if (!dest || !path || !interface || !method || !resmsg)
        goto compose_error;

    dbusmsg = dbus_message_new_method_call(dest, path, interface, method);

    if (dbusmsg == NULL)
        goto compose_error;

    switch (resmsg->type) {

    default:
        success = FALSE;
        break;

    case RESMSG_REGISTER:
    case RESMSG_UPDATE:
        record  = &resmsg->record;
        success = dbus_message_append_args(dbusmsg,
                                 DBUS_TYPE_INT32 , &record->type,
                                 DBUS_TYPE_UINT32, &record->id,
                                 DBUS_TYPE_UINT32, &record->reqno,
                                 DBUS_TYPE_UINT32, &record->rset.all,
                                 DBUS_TYPE_UINT32, &record->rset.opt,
                                 DBUS_TYPE_UINT32, &record->rset.share,
                                 DBUS_TYPE_UINT32, &record->rset.mask,
                                 DBUS_TYPE_STRING,  record->klass ?
                                                   &record->klass : &empty_str,
                                 DBUS_TYPE_UINT32, &record->mode,
                                 DBUS_TYPE_INVALID);
        break;

    case RESMSG_UNREGISTER:
    case RESMSG_ACQUIRE:
    case RESMSG_RELEASE:
        possess = &resmsg->possess;
        success = dbus_message_append_args(dbusmsg,
                                           DBUS_TYPE_INT32 , &possess->type,
                                           DBUS_TYPE_UINT32, &possess->id,
                                           DBUS_TYPE_UINT32, &possess->reqno,
                                           DBUS_TYPE_INVALID);
        break;

    case RESMSG_GRANT:
    case RESMSG_ADVICE:
        notify  = &resmsg->notify;
        success = dbus_message_append_args(dbusmsg,
                                           DBUS_TYPE_INT32 , &notify->type,
                                           DBUS_TYPE_UINT32, &notify->id,
                                           DBUS_TYPE_UINT32, &notify->reqno,
                                           DBUS_TYPE_UINT32, &notify->resrc,
                                           DBUS_TYPE_INVALID);
        break;

    case RESMSG_AUDIO:
        audio    = &resmsg->audio;
        property = &audio->property;
        success  = dbus_message_append_args(dbusmsg,
                       DBUS_TYPE_INT32 , &audio->type,
                       DBUS_TYPE_UINT32, &audio->id,
                       DBUS_TYPE_UINT32, &audio->reqno,
                       DBUS_TYPE_STRING,  audio->group ?
                                         &audio->group : &empty_str,
                       DBUS_TYPE_UINT32, &audio->pid,
                       DBUS_TYPE_STRING,  property->name ?
                                         &property->name : &empty_str,
                       DBUS_TYPE_INT32 , &property->match.method,
                       DBUS_TYPE_STRING,  property->match.pattern ?
                                         &property->match.pattern : &empty_str,
                       DBUS_TYPE_INVALID);
        break;

    case RESMSG_VIDEO:
        video   = &resmsg->video;
        success = dbus_message_append_args(dbusmsg,
                       DBUS_TYPE_INT32 , &video->type,
                       DBUS_TYPE_UINT32, &video->id,
                       DBUS_TYPE_UINT32, &video->reqno,
                       DBUS_TYPE_UINT32, &video->pid,
                       DBUS_TYPE_INVALID);
        break;
    }

    if (!success)
        goto compose_error;

    return dbusmsg;

 compose_error:
    if (dbusmsg != NULL)
        dbus_message_unref(dbusmsg);

    return NULL;
}

DBusMessage *resmsg_dbus_reply_message(DBusMessage *dbusmsg,resmsg_t *resreply)
{
    static const char *empty_str = "";

    DBusMessage       *dbusreply;
    resmsg_status_t   *status;
    int                success;

    if (!dbusmsg || !resreply || resreply->type != RESMSG_STATUS)
        return NULL;
    

    status    = &resreply->status;
    dbusreply = dbus_message_new_method_return(dbusmsg);
    success   = dbus_message_append_args(dbusreply,
                                         DBUS_TYPE_INT32 , &status->type,
                                         DBUS_TYPE_UINT32, &status->id,
                                         DBUS_TYPE_UINT32, &status->reqno,
                                         DBUS_TYPE_INT32 , &status->errcod,
                                         DBUS_TYPE_STRING,  status->errmsg ?
                                                 &status->errmsg : &empty_str,
                                         DBUS_TYPE_INVALID);
    if (!success) {
        dbus_message_unref(dbusreply);
        dbusreply = NULL;
    }

    return dbusreply;
}

resmsg_t *resmsg_dbus_parse_message(DBusMessage *dbusmsg, resmsg_t *resmsg)
{
    int32_t            type;
    resmsg_record_t   *record;
    resmsg_possess_t  *possess;
    resmsg_notify_t   *notify;
    resmsg_audio_t    *audio;
    resmsg_video_t    *video;
    resmsg_status_t   *status;
    resmsg_property_t *property;
    resmsg_match_t    *match;
    int                free_resmsg;
    int                success;
    
    if (dbusmsg == NULL) {
        free_resmsg = FALSE;
        goto parse_error;
    }

    /*
     * make sure we have a valid structure to populate with data
     */
    if (resmsg != NULL)
        free_resmsg = FALSE;
    else {
        free_resmsg = TRUE;
        resmsg = malloc(sizeof(resmsg_t));
    }

    if (resmsg == NULL)
        goto parse_error;

    memset(resmsg, 0, sizeof(resmsg_t));


    /*
     * first get the type
     */
    success = dbus_message_get_args(dbusmsg, NULL,
                                    DBUS_TYPE_INT32, &type,
                                    DBUS_TYPE_INVALID);
    if (!success)
        goto parse_error;


    /*
     * parse the whole message
     */
    switch (type) {

    default:
        success = FALSE;
        type    = RESMSG_INVALID;
        break;

    case RESMSG_REGISTER:
    case RESMSG_UPDATE:
        record  = &resmsg->record;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &record->type,
                                        DBUS_TYPE_UINT32, &record->id,
                                        DBUS_TYPE_UINT32, &record->reqno,
                                        DBUS_TYPE_UINT32, &record->rset.all,
                                        DBUS_TYPE_UINT32, &record->rset.opt,
                                        DBUS_TYPE_UINT32, &record->rset.share,
                                        DBUS_TYPE_UINT32, &record->rset.mask,
                                        DBUS_TYPE_STRING, &record->klass,
                                        DBUS_TYPE_UINT32, &record->mode,
                                        DBUS_TYPE_INVALID);
        break;

    case RESMSG_UNREGISTER:
    case RESMSG_ACQUIRE:
    case RESMSG_RELEASE:
        possess = &resmsg->possess;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &possess->type,
                                        DBUS_TYPE_UINT32, &possess->id,
                                        DBUS_TYPE_UINT32, &possess->reqno,
                                        DBUS_TYPE_INVALID);
        break;


    case RESMSG_GRANT:
    case RESMSG_ADVICE:
        notify  = &resmsg->notify;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &notify->type,
                                        DBUS_TYPE_UINT32, &notify->id,
                                        DBUS_TYPE_UINT32, &notify->reqno,
                                        DBUS_TYPE_UINT32, &notify->resrc,
                                        DBUS_TYPE_INVALID);
        break;

    case RESMSG_AUDIO:
        audio    = &resmsg->audio;
        property = &audio->property;
        match    = &property->match;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &audio->type,
                                        DBUS_TYPE_UINT32, &audio->id,
                                        DBUS_TYPE_UINT32, &audio->reqno,
                                        DBUS_TYPE_STRING, &audio->group,
                                        DBUS_TYPE_UINT32, &audio->pid,
                                        DBUS_TYPE_STRING, &property->name,
                                        DBUS_TYPE_INT32 , &match->method,
                                        DBUS_TYPE_STRING, &match->pattern,
                                        DBUS_TYPE_INVALID);
        break;

    case RESMSG_VIDEO:
        video   = &resmsg->video;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &video->type,
                                        DBUS_TYPE_UINT32, &video->id,
                                        DBUS_TYPE_UINT32, &video->reqno,
                                        DBUS_TYPE_UINT32, &video->pid,
                                        DBUS_TYPE_INVALID);
        break;

    case RESMSG_STATUS:
        status  = &resmsg->status;
        success = dbus_message_get_args(dbusmsg, NULL,
                                        DBUS_TYPE_INT32 , &status->type,
                                        DBUS_TYPE_UINT32, &status->id,
                                        DBUS_TYPE_UINT32, &status->reqno,
                                        DBUS_TYPE_INT32 , &status->errcod,
                                        DBUS_TYPE_STRING, &status->errmsg,
                                        DBUS_TYPE_INVALID);
        break;
    }
        
    if (!success)
        goto parse_error;

    /* everything looks OK */
    return resmsg;

    /* something went wrong */
 parse_error:
    if (resmsg != NULL && free_resmsg)
        free(resmsg);

    return NULL;
}





/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
