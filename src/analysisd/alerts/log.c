/* Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include "log.h"
#include "alerts.h"
#include "getloglocation.h"
#include "rules.h"
#include "eventinfo.h"
#include "config.h"


/* Drop/allow patterns */
static OSMatch FWDROPpm;
static OSMatch FWALLOWpm;

/* Allow custom alert output tokens */
typedef enum e_custom_alert_tokens_id {
    CUSTOM_ALERT_TOKEN_TIMESTAMP = 0,
    CUSTOM_ALERT_TOKEN_FTELL,
    CUSTOM_ALERT_TOKEN_RULE_ALERT_OPTIONS,
    CUSTOM_ALERT_TOKEN_HOSTNAME,
    CUSTOM_ALERT_TOKEN_LOCATION,
    CUSTOM_ALERT_TOKEN_RULE_ID,
    CUSTOM_ALERT_TOKEN_RULE_LEVEL,
    CUSTOM_ALERT_TOKEN_RULE_COMMENT,
    CUSTOM_ALERT_TOKEN_SRC_IP,
    CUSTOM_ALERT_TOKEN_DST_USER,
    CUSTOM_ALERT_TOKEN_FULL_LOG,
    CUSTOM_ALERT_TOKEN_RULE_GROUP,
    CUSTOM_ALERT_TOKEN_LAST
} CustomAlertTokenID;

static const char CustomAlertTokenName[CUSTOM_ALERT_TOKEN_LAST][15] = {
    { "$TIMESTAMP" },
    { "$FTELL" },
    { "$RULEALERT" },
    { "$HOSTNAME" },
    { "$LOCATION" },
    { "$RULEID" },
    { "$RULELEVEL" },
    { "$RULECOMMENT" },
    { "$SRCIP" },
    { "$DSTUSER" },
    { "$FULLLOG" },
    { "$RULEGROUP" },
};

/* Store the events in a file
 * The string must be null terminated and contain
 * any necessary new lines, tabs, etc.
 */
void OS_Store(const Eventinfo *lf)
{
    if (strcmp(lf->location, "ossec-keepalive") == 0) {
        return;
    }
    if (strstr(lf->location, "->ossec-keepalive") != NULL) {
        return;
    }

    fprintf(_eflog,
            "%d %s %02d %s %s%s%s %s\n",
            lf->year,
            lf->mon,
            lf->day,
            lf->hour,
            lf->hostname != lf->location ? lf->hostname : "",
            lf->hostname != lf->location ? "->" : "",
            lf->location,
            lf->full_log);

    fflush(_eflog);
    return;
}

void OS_LogOutput(Eventinfo *lf)
{
    int i;

#ifdef LIBGEOIP_ENABLED
    if (Config.geoipdb_file) {
        if (lf->srcip) {
                lf->srcgeoip = GetGeoInfobyIP(lf->srcip);
        }
        if (lf->dstip) {
                lf->dstgeoip = GetGeoInfobyIP(lf->dstip);
        }
    }
#endif

    printf(
        "** Alert %ld.%ld:%s - %s\n"
        "%d %s %02d %s %s%s%s\nRule: %d (level %d) -> '%s'"
        "%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n%.1256s\n",
        (long int)lf->time,
        __crt_ftell,
        lf->generated_rule->alert_opts & DO_MAILALERT ? " mail " : "",
        lf->generated_rule->group,
        lf->year,
        lf->mon,
        lf->day,
        lf->hour,
        lf->hostname != lf->location ? lf->hostname : "",
        lf->hostname != lf->location ? "->" : "",
        lf->location,
        lf->generated_rule->sigid,
        lf->generated_rule->level,
        lf->comment,

        lf->srcip == NULL ? "" : "\nSrc IP: ",
        lf->srcip == NULL ? "" : lf->srcip,

#ifdef LIBGEOIP_ENABLED
        lf->srcgeoip == NULL ? "" : "\nSrc Location: ",
        lf->srcgeoip == NULL ? "" : lf->srcgeoip,
#else
        "",
        "",
#endif



        lf->srcport == NULL ? "" : "\nSrc Port: ",
        lf->srcport == NULL ? "" : lf->srcport,

        lf->dstip == NULL ? "" : "\nDst IP: ",
        lf->dstip == NULL ? "" : lf->dstip,

#ifdef LIBGEOIP_ENABLED
        lf->dstgeoip == NULL ? "" : "\nDst Location: ",
        lf->dstgeoip == NULL ? "" : lf->dstgeoip,
#else
        "",
        "",
#endif



        lf->dstport == NULL ? "" : "\nDst Port: ",
        lf->dstport == NULL ? "" : lf->dstport,

        lf->dstuser == NULL ? "" : "\nUser: ",
        lf->dstuser == NULL ? "" : lf->dstuser,

        lf->full_log);

    /* FIM events */

    if (lf->filename) {
        printf("File: %s\n", lf->filename);

        if (lf->size_before)
            printf("Old size: %s\n", lf->size_before);
        if (lf->size_after)
            printf("New size: %s\n", lf->size_after);

        if (lf->perm_before)
            printf("Old permissions: %6o\n", lf->perm_before);
        if (lf->perm_after)
            printf("New permissions: %6o\n", lf->perm_after);

        if (lf->owner_before) {
            if (lf->uname_before)
                printf("Old user: %s (%s)\n", lf->uname_before, lf->owner_before);
            else
                printf("Old user: %s\n", lf->owner_before);
        }
        if (lf->owner_after) {
            if (lf->uname_after)
                printf("New user: %s (%s)\n", lf->uname_after, lf->owner_after);
            else
                printf("New user: %s\n", lf->owner_after);
        }

        if (lf->gowner_before) {
            if (lf->gname_before)
                printf("Old group: %s (%s)\n", lf->gname_before, lf->gowner_before);
            else
                printf("Old group: %s\n", lf->gowner_before);
        }
        if (lf->gowner_after) {
            if (lf->gname_after)
                printf("New group: %s (%s)\n", lf->gname_after, lf->gowner_after);
            else
                printf("New group: %s\n", lf->gowner_after);
        }

        if (lf->md5_before)
            printf("Old MD5: %s\n", lf->md5_before);
        if (lf->md5_after)
            printf("New MD5: %s\n", lf->md5_after);


        if (lf->sha1_before)
            printf("Old SHA1: %s\n", lf->sha1_before);
        if (lf->sha1_after)
            printf("New SHA1: %s\n", lf->sha1_after);

        if (lf->mtime_before)
            printf("Old date: %s", ctime(&lf->mtime_before));
        if (lf->mtime_after)
            printf("New date: %s", ctime(&lf->mtime_after));

        if (lf->inode_before)
            printf("Old inode: %ld\n", lf->inode_before);
        if (lf->inode_after)
            printf("New inode: %ld\n", lf->inode_after);
    }

    // Dynamic fields, except for syscheck events
    if (lf->decoder_info->fields && !lf->filename) {
        for (i = 0; i < Config.decoder_order_size; i++) {
            if (lf->decoder_info->fields[i] && lf->fields[i]) {
                printf("%s: %s\n", lf->decoder_info->fields[i], lf->fields[i]);
            }
        }
    }

    /* Print the last events if present */
    if (lf->generated_rule->last_events) {
        char **lasts = lf->generated_rule->last_events;
        while (*lasts) {
            printf("%.1256s\n", *lasts);
            lasts++;
        }
        lf->generated_rule->last_events[0] = NULL;
    }

    printf("\n");

    fflush(stdout);
    return;
}

void OS_Log(Eventinfo *lf)
{
    int i;

#ifdef LIBGEOIP_ENABLED
    if (Config.geoipdb_file) {
        if (lf->srcip) {
                lf->srcgeoip = GetGeoInfobyIP(lf->srcip);
        }
        if (lf->dstip) {
                lf->dstgeoip = GetGeoInfobyIP(lf->dstip);
        }
    }
#endif

    /* Writing to the alert log file */
    fprintf(_aflog,
            "** Alert %ld.%ld:%s - %s\n"
            "%d %s %02d %s %s%s%s\nRule: %d (level %d) -> '%s'"
            "%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n%.1256s\n",
            (long int)lf->time,
            __crt_ftell,
            lf->generated_rule->alert_opts & DO_MAILALERT ? " mail " : "",
            lf->generated_rule->group,
            lf->year,
            lf->mon,
            lf->day,
            lf->hour,
            lf->hostname != lf->location ? lf->hostname : "",
            lf->hostname != lf->location ? "->" : "",
            lf->location,
            lf->generated_rule->sigid,
            lf->generated_rule->level,
            lf->comment,

            lf->srcip == NULL ? "" : "\nSrc IP: ",
            lf->srcip == NULL ? "" : lf->srcip,

#ifdef LIBGEOIP_ENABLED
            lf->srcgeoip == NULL ? "" : "\nSrc Location: ",
            lf->srcgeoip == NULL ? "" : lf->srcgeoip,
#else
            "",
            "",
#endif


            lf->srcport == NULL ? "" : "\nSrc Port: ",
            lf->srcport == NULL ? "" : lf->srcport,

            lf->dstip == NULL ? "" : "\nDst IP: ",
            lf->dstip == NULL ? "" : lf->dstip,

#ifdef LIBGEOIP_ENABLED
            lf->dstgeoip == NULL ? "" : "\nDst Location: ",
            lf->dstgeoip == NULL ? "" : lf->dstgeoip,
#else
            "",
            "",
#endif



            lf->dstport == NULL ? "" : "\nDst Port: ",
            lf->dstport == NULL ? "" : lf->dstport,

            lf->dstuser == NULL ? "" : "\nUser: ",
            lf->dstuser == NULL ? "" : lf->dstuser,

            lf->full_log);

    /* FIM events */

    if (lf->filename) {
        fprintf(_aflog, "File: %s\n", lf->filename);

        if (lf->size_before)
            fprintf(_aflog, "Old size: %s\n", lf->size_before);
        if (lf->size_after)
            fprintf(_aflog, "New size: %s\n", lf->size_after);

        if (lf->perm_before)
            fprintf(_aflog, "Old permissions: %6o\n", lf->perm_before);
        if (lf->perm_after)
            fprintf(_aflog, "New permissions: %6o\n", lf->perm_after);

        if (lf->owner_before) {
            if (lf->uname_before)
                fprintf(_aflog, "Old user: %s (%s)\n", lf->uname_before, lf->owner_before);
            else
                fprintf(_aflog, "Old user: %s\n", lf->owner_before);
        }
        if (lf->owner_after) {
            if (lf->uname_after)
                fprintf(_aflog, "New user: %s (%s)\n", lf->uname_after, lf->owner_after);
            else
                fprintf(_aflog, "New user: %s\n", lf->owner_after);
        }

        if (lf->gowner_before) {
            if (lf->gname_before)
                fprintf(_aflog, "Old group: %s (%s)\n", lf->gname_before, lf->gowner_before);
            else
                fprintf(_aflog, "Old group: %s\n", lf->gowner_before);
        }
        if (lf->gowner_after) {
            if (lf->gname_after)
                fprintf(_aflog, "New group: %s (%s)\n", lf->gname_after, lf->gowner_after);
            else
                fprintf(_aflog, "New group: %s\n", lf->gowner_after);
        }

        if (lf->md5_before)
            fprintf(_aflog, "Old MD5: %s\n", lf->md5_before);
        if (lf->md5_after)
            fprintf(_aflog, "New MD5: %s\n", lf->md5_after);


        if (lf->sha1_before)
            fprintf(_aflog, "Old SHA1: %s\n", lf->sha1_before);
        if (lf->sha1_after)
            fprintf(_aflog, "New SHA1: %s\n", lf->sha1_after);

        if (lf->mtime_before)
            fprintf(_aflog, "Old date: %s", ctime(&lf->mtime_before));
        if (lf->mtime_after)
            fprintf(_aflog, "New date: %s", ctime(&lf->mtime_after));

        if (lf->inode_before)
            fprintf(_aflog, "Old inode: %ld\n", lf->inode_before);
        if (lf->inode_after)
            fprintf(_aflog, "New inode: %ld\n", lf->inode_after);
    }

    // Dynamic fields, except for syscheck events
    if (lf->decoder_info->fields && !lf->filename) {
        for (i = 0; i < Config.decoder_order_size; i++) {
            if (lf->decoder_info->fields[i] && lf->fields[i]) {
                fprintf(_aflog, "%s: %s\n", lf->decoder_info->fields[i], lf->fields[i]);
            }
        }
    }

    /* Print the last events if present */
    if (lf->generated_rule->last_events) {
        char **lasts = lf->generated_rule->last_events;
        while (*lasts) {
            fprintf(_aflog, "%.1256s\n", *lasts);
            lasts++;
        }
        lf->generated_rule->last_events[0] = NULL;
    }

    fprintf(_aflog, "\n");
    fflush(_aflog);

    return;
}

void OS_CustomLog(const Eventinfo *lf, const char *format)
{
    char *log;
    char *tmp_log;
    char tmp_buffer[1024];

    /* Replace all the tokens */
    os_strdup(format, log);

    snprintf(tmp_buffer, 1024, "%ld", (long int)lf->time);
    tmp_log = searchAndReplace(log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_TIMESTAMP], tmp_buffer);
    if (log) {
        os_free(log);
        log = NULL;
    }
    snprintf(tmp_buffer, 1024, "%ld", __crt_ftell);
    log = searchAndReplace(tmp_log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_FTELL], tmp_buffer);
    if (tmp_log) {
        os_free(tmp_log);
        tmp_log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%s", (lf->generated_rule->alert_opts & DO_MAILALERT) ? "mail " : "");
    tmp_log = searchAndReplace(log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_RULE_ALERT_OPTIONS], tmp_buffer);
    if (log) {
        os_free(log);
        log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%s", lf->hostname ? lf->hostname : "None");
    log = searchAndReplace(tmp_log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_HOSTNAME], tmp_buffer);
    if (tmp_log) {
        os_free(tmp_log);
        tmp_log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%s", lf->location ? lf->location : "None");
    tmp_log = searchAndReplace(log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_LOCATION], tmp_buffer);
    if (log) {
        os_free(log);
        log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%d", lf->generated_rule->sigid);
    log = searchAndReplace(tmp_log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_RULE_ID], tmp_buffer);
    if (tmp_log) {
        os_free(tmp_log);
        tmp_log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%d", lf->generated_rule->level);
    tmp_log = searchAndReplace(log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_RULE_LEVEL], tmp_buffer);
    if (log) {
        os_free(log);
        log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%s", lf->srcip ? lf->srcip : "None");
    log = searchAndReplace(tmp_log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_SRC_IP], tmp_buffer);
    if (tmp_log) {
        os_free(tmp_log);
        tmp_log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%s", lf->dstuser ? lf->dstuser : "None");

    tmp_log = searchAndReplace(log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_DST_USER], tmp_buffer);
    if (log) {
        os_free(log);
        log = NULL;
    }
    char *escaped_log;
    escaped_log = escape_newlines(lf->full_log);

    log = searchAndReplace(tmp_log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_FULL_LOG], escaped_log );
    if (tmp_log) {
        os_free(tmp_log);
        tmp_log = NULL;
    }

    if (escaped_log) {
        os_free(escaped_log);
        escaped_log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%s", lf->comment ? lf->comment : "");
    tmp_log = searchAndReplace(log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_RULE_COMMENT], tmp_buffer);
    if (log) {
        os_free(log);
        log = NULL;
    }

    snprintf(tmp_buffer, 1024, "%s", lf->generated_rule->group ? lf->generated_rule->group : "");
    log = searchAndReplace(tmp_log, CustomAlertTokenName[CUSTOM_ALERT_TOKEN_RULE_GROUP], tmp_buffer);
    if (tmp_log) {
        os_free(tmp_log);
        tmp_log = NULL;
    }

    fprintf(_aflog, "%s", log);
    fprintf(_aflog, "\n");
    fflush(_aflog);

    if (log) {
        os_free(log);
        log = NULL;
    }

    return;
}

void OS_InitFwLog()
{
    /* Initialize fw log regexes */
    if (!OSMatch_Compile(FWDROP, &FWDROPpm, 0)) {
        ErrorExit(REGEX_COMPILE, ARGV0, FWDROP,
                  FWDROPpm.error);
    }

    if (!OSMatch_Compile(FWALLOW, &FWALLOWpm, 0)) {
        ErrorExit(REGEX_COMPILE, ARGV0, FWALLOW,
                  FWALLOWpm.error);
    }
}

int FW_Log(Eventinfo *lf)
{
    /* If we don't have the srcip or the
     * action, there is no point in going
     * forward over here
     */
    if (!lf->action || !lf->srcip || !lf->dstip || !lf->srcport ||
            !lf->dstport || !lf->protocol) {
        return (0);
    }

    /* Set the actions */
    switch (*lf->action) {
        /* discard, drop, deny, */
        case 'd':
        case 'D':
        /* reject, */
        case 'r':
        case 'R':
        /* block */
        case 'b':
        case 'B':
            os_free(lf->action);
            os_strdup("DROP", lf->action);
            break;
        /* Closed */
        case 'c':
        case 'C':
        /* Teardown */
        case 't':
        case 'T':
            os_free(lf->action);
            os_strdup("CLOSED", lf->action);
            break;
        /* allow, accept, */
        case 'a':
        case 'A':
        /* pass/permitted */
        case 'p':
        case 'P':
        /* open */
        case 'o':
        case 'O':
            os_free(lf->action);
            os_strdup("ALLOW", lf->action);
            break;
        default:
            if (OSMatch_Execute(lf->action, strlen(lf->action), &FWDROPpm)) {
                os_free(lf->action);
                os_strdup("DROP", lf->action);
            }
            if (OSMatch_Execute(lf->action, strlen(lf->action), &FWALLOWpm)) {
                os_free(lf->action);
                os_strdup("ALLOW", lf->action);
            } else {
                os_free(lf->action);
                os_strdup("UNKNOWN", lf->action);
            }
            break;
    }

    /* Log to file */
    fprintf(_fflog,
            "%d %s %02d %s %s%s%s %s %s %s:%s->%s:%s\n",
            lf->year,
            lf->mon,
            lf->day,
            lf->hour,
            lf->hostname != lf->location ? lf->hostname : "",
            lf->hostname != lf->location ? "->" : "",
            lf->location,
            lf->action,
            lf->protocol,
            lf->srcip,
            lf->srcport,
            lf->dstip,
            lf->dstport);

    fflush(_fflog);

    return (1);
}
