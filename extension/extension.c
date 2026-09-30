//coontrol fiel is liek metadat fo the extesion as toinformation about teh extension 
//in make file module is the c file anem extsionwaht to callt hsi extensionad da t is slq file anme
#include "postgres.h"
#include "executor/executor.h"
#include "fmgr.h"
#include "miscadmin.h"

PG_MODULE_MAGIC;
PGDLLEXPORT void DBAnalyzerWorker(Datum main_arg);


// This is used to store the previous hook of some other library.
// In PostgreSQL, each extension can have its own hook. To make sure each
// extension's code executes, we store the function pointer and then use it
// to maintain the hook chain.

// This stores the previous ExecutorStart hook.

static ExecutorStart_hook_type prev_ExecutorStart_hook = NULL;
static ExecutorEnd_hook_type prev_ExecutorEnd_hook = NULL;
// ExecutorStart_hook_type is a typedef (alias) for a function-pointer type.
// It represents a function that returns void and takes:
// (QueryDesc *, int) as params
//
// Conceptually:
// void (*function_pointer)(QueryDesc *, int)
//
// So prev_ExecutorStart_hook can point to a function
// with exactly this signature.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

typedef struct {
    bool success;
    int socket_fd;
} SocketResult;

static SocketResult socketResult = {
    .success = false,
    .socket_fd = -1
};

static char socketPath[] = "/tmp/my_socket";


static SocketResult CreateClientSocket() {
    SocketResult result = {
        .success = false,
        .socket_fd = -1
    };
    int socket_fd;
    struct sockaddr_un addr;

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        elog(WARNING, "Extension: failed to create socket: %m");
        return result;
    }

    
    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path,socketPath);

    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        elog(WARNING, "Extension: failed to connect to Unix socket: %m");
        close(socket_fd);
        return result;
    }

    result.success = true;
    result.socket_fd = socket_fd;

    return result;
}


static bool SendAll(int socket_fd, const void *data, size_t length)
{
    const char *buffer = data;
    size_t sent = 0;

    while (sent < length)
    {
        ssize_t result = send(socket_fd, buffer + sent, length - sent, 0);

        if (result <= 0)
            return false;

        sent += result;
    }

    return true;
}


static void HandleJsonMessage(const char *json)
{
    uint64_t jsonLength;


    if (!socketResult.success)
    {
        elog(LOG, "Extension: Unix socket not found, dropping metrics");
        return;
    }

    jsonLength = strlen(json);

    if (!SendAll(socketResult.socket_fd, &jsonLength, sizeof(jsonLength)))
    {
        elog(WARNING, "Extension: failed to send JSON length: %m");

        close(socketResult.socket_fd);

        socketResult.success = false;
        socketResult.socket_fd = -1;

        return;
    }

    if (!SendAll(socketResult.socket_fd, json, jsonLength))
    {
        elog(WARNING, "Extension: failed to send JSON message: %m");

        close(socketResult.socket_fd);

        socketResult.success = false;
        socketResult.socket_fd = -1;

        return;
    }

    // elog(LOG, "Extension: JSON metrics sent successfully");

    elog(LOG, "Extension: JSON sent successfully: %s", json);
}


#include "utils/jsonb.h"
PG_FUNCTION_INFO_V1(backend_command);
Datum backend_command(PG_FUNCTION_ARGS)
{
    Jsonb *jsonb_data;
    char *json_string;

    jsonb_data = PG_GETARG_JSONB_P(0);

    json_string = JsonbToCString(NULL, &jsonb_data->root, VARSIZE_ANY_EXHDR(jsonb_data));

    HandleJsonMessage(json_string);

    pfree(json_string);

    PG_RETURN_VOID();
}


#include "libpq/libpq-be.h"
typedef struct
{
    // Connection / session information
    int pid;
    const char *database_name;
    const char *user_name;
    const char *application_name;
    const char *remote_host;
    const char *remote_port;
    pg_time_t backend_start;

    // Query information
    const char *query;
    CmdType operation;
    ParamListInfo params;

    // PostgreSQL QueryDesc information
    bool already_executed;

    // Hook information
    const char *operation_name;

} QueryStartInfo;

static void LogQueryStartInfo(QueryStartInfo *info)
{
    elog(LOG, "========== QUERY START ==========");

    elog(LOG, "PID = %d", info->pid);
    elog(LOG, "DATABASE = %s", info->database_name);
    elog(LOG, "USER = %s", info->user_name);
    elog(LOG, "APPLICATION = %s", info->application_name);
    elog(LOG, "CLIENT HOST = %s", info->remote_host);
    elog(LOG, "CLIENT PORT = %s", info->remote_port);
    elog(LOG, "BACKEND START = %ld", info->backend_start);

    elog(LOG, "QUERY = %s", info->query);
    elog(LOG, "OPERATION = %d", info->operation);
    elog(LOG, "ALREADY EXECUTED = %s", info->already_executed ? "true" : "false");

    elog(LOG, "================================");
}



static void SetConnectionInfo(QueryStartInfo *info)
{
    info->pid = MyProcPid;
    info->backend_start = MyStartTime;

    if (MyProcPort)
    {
        info->database_name = MyProcPort->database_name;
        info->user_name = MyProcPort->user_name;
        info->application_name = MyProcPort->application_name;
        info->remote_host = MyProcPort->remote_host;
        info->remote_port = MyProcPort->remote_port;
    }
}

static void SetQueryInfo(QueryStartInfo *info, QueryDesc *queryDesc)
{
    info->query = queryDesc->sourceText;
    info->operation = queryDesc->operation;
    info->params = queryDesc->params;
    info->already_executed = queryDesc->already_executed;

}


#include "catalog/pg_type.h"
#include "nodes/params.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/json.h"
static void AppendParamsJson(StringInfo json, ParamListInfo params)
{
    int i;
    ParamExternData *param;
    Oid typoutput;
    bool typisvarlena;
    char *value;
    
    appendStringInfoString(json, "\"params\":[");

    if (params == NULL)
    {
        appendStringInfoChar(json, ']');
        return;
    }

    for (i = 0; i < params->numParams; i++)
    {
        param = &params->params[i];

        if (i > 0)
            appendStringInfoChar(json, ',');

        appendStringInfoChar(json, '{');

        appendStringInfo(json, "\"param_id\":%d,", i + 1);

        appendStringInfo(json, "\"type_oid\":%u,", param->ptype);

        appendStringInfo(json, "\"is_null\":%s,",
                         param->isnull ? "true" : "false");

        appendStringInfoString(json, "\"value\":");

        if (param->isnull)
        {
            appendStringInfoString(json, "null");
        }
        else if (param->ptype == InvalidOid)
        {
            appendStringInfoString(json, "null");
        }
        else
        {
            

            getTypeOutputInfo(param->ptype, &typoutput, &typisvarlena);

            value = OidOutputFunctionCall(typoutput, param->value);

            escape_json(json, value);

            pfree(value);
        }

        appendStringInfoChar(json, '}');
    }

    appendStringInfoChar(json, ']');
}


static char *QueryStartInfoToJson(QueryStartInfo *info)
{
    StringInfoData json;

    initStringInfo(&json);

    appendStringInfoChar(&json, '{');

    appendStringInfo(&json, "\"operation_name\":");
    escape_json(&json, info->operation_name ? info->operation_name : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"pid\":%d,", info->pid);

    appendStringInfo(&json, "\"database_name\":");
    escape_json(&json, info->database_name ? info->database_name : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"user_name\":");
    escape_json(&json, info->user_name ? info->user_name : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"application_name\":");
    escape_json(&json, info->application_name ? info->application_name : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"remote_host\":");
    escape_json(&json, info->remote_host ? info->remote_host : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"remote_port\":");
    escape_json(&json, info->remote_port ? info->remote_port : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"backend_start\":%ld,", info->backend_start);

    appendStringInfo(&json, "\"query\":");
    escape_json(&json, info->query ? info->query : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"operation\":%d,", info->operation);

    appendStringInfo(&json, "\"already_executed\":%s,",
                     info->already_executed ? "true" : "false");

    AppendParamsJson(&json, info->params);

    appendStringInfoChar(&json, '}');

    return json.data;
}




static void Extension_executor(QueryDesc *queryDesc, int eflags)
{
    QueryStartInfo info = {0};
    char *message;

    info.operation_name = "ExecutorStart";

    queryDesc->instrument_options |= INSTRUMENT_TIMER;
    queryDesc->instrument_options |= INSTRUMENT_BUFFERS;
    queryDesc->instrument_options |= INSTRUMENT_WAL;

    queryDesc->totaltime = InstrAlloc(1, queryDesc->instrument_options, false);
    

    if (prev_ExecutorStart_hook)
        prev_ExecutorStart_hook(queryDesc, eflags);
    else
        standard_ExecutorStart(queryDesc, eflags);
    

    SetConnectionInfo(&info);
    
    SetQueryInfo(&info, queryDesc);

    // LogQueryStartInfo(&info);

    message = QueryStartInfoToJson(&info);

    // elog(LOG, "JSON = %s", message);

    HandleJsonMessage(message);

    pfree(message);


}


#include "executor/instrument.h"

typedef struct
{
    // Connection / session information
    int pid;
    const char *database_name;
    const char *user_name;
    const char *application_name;
    const char *remote_host;
    const char *remote_port;
    pg_time_t backend_start;

    // Query information
    const char *query;
    CmdType operation;
    ParamListInfo params;
    bool already_executed;

    // Execution information
    double execution_time_ms;
    uint64 rows_processed;

    // Shared buffers
    int64 shared_blks_hit;
    int64 shared_blks_read;
    int64 shared_blks_dirtied;
    int64 shared_blks_written;

    // Local buffers
    int64 local_blks_hit;
    int64 local_blks_read;
    int64 local_blks_dirtied;
    int64 local_blks_written;

    // Temporary buffers
    int64 temp_blks_read;
    int64 temp_blks_written;

    // Buffer I/O time
    double shared_blk_read_time_ms;
    double shared_blk_write_time_ms;
    double local_blk_read_time_ms;
    double local_blk_write_time_ms;
    double temp_blk_read_time_ms;
    double temp_blk_write_time_ms;

    // WAL
    int64 wal_records;
    int64 wal_fpi;
    uint64 wal_bytes;
    int64 wal_buffers_full;

    // Hook information
    const char *operation_name;

} QueryEndInfo;

static void SetEndExecutionInfo(QueryEndInfo *info, QueryDesc *queryDesc)
{
    info->execution_time_ms = 0;
    info->rows_processed = 0;

    info->shared_blks_hit = 0;
    info->shared_blks_read = 0;
    info->shared_blks_dirtied = 0;
    info->shared_blks_written = 0;

    info->local_blks_hit = 0;
    info->local_blks_read = 0;
    info->local_blks_dirtied = 0;
    info->local_blks_written = 0;

    info->temp_blks_read = 0;
    info->temp_blks_written = 0;

    info->shared_blk_read_time_ms = 0;
    info->shared_blk_write_time_ms = 0;
    info->local_blk_read_time_ms = 0;
    info->local_blk_write_time_ms = 0;
    info->temp_blk_read_time_ms = 0;
    info->temp_blk_write_time_ms = 0;

    info->wal_records = 0;
    info->wal_fpi = 0;
    info->wal_bytes = 0;
    info->wal_buffers_full = 0;

    if (queryDesc->totaltime)
    {
        Instrumentation *instr = queryDesc->totaltime;

        info->execution_time_ms =
            INSTR_TIME_GET_MILLISEC(instr->counter);

        info->shared_blks_hit = instr->bufusage.shared_blks_hit;
        info->shared_blks_read = instr->bufusage.shared_blks_read;
        info->shared_blks_dirtied = instr->bufusage.shared_blks_dirtied;
        info->shared_blks_written = instr->bufusage.shared_blks_written;

        info->local_blks_hit = instr->bufusage.local_blks_hit;
        info->local_blks_read = instr->bufusage.local_blks_read;
        info->local_blks_dirtied = instr->bufusage.local_blks_dirtied;
        info->local_blks_written = instr->bufusage.local_blks_written;

        info->temp_blks_read = instr->bufusage.temp_blks_read;
        info->temp_blks_written = instr->bufusage.temp_blks_written;

        info->shared_blk_read_time_ms =
            INSTR_TIME_GET_MILLISEC(instr->bufusage.shared_blk_read_time);
        info->shared_blk_write_time_ms =
            INSTR_TIME_GET_MILLISEC(instr->bufusage.shared_blk_write_time);
        info->local_blk_read_time_ms =
            INSTR_TIME_GET_MILLISEC(instr->bufusage.local_blk_read_time);
        info->local_blk_write_time_ms =
            INSTR_TIME_GET_MILLISEC(instr->bufusage.local_blk_write_time);
        info->temp_blk_read_time_ms =
            INSTR_TIME_GET_MILLISEC(instr->bufusage.temp_blk_read_time);
        info->temp_blk_write_time_ms =
            INSTR_TIME_GET_MILLISEC(instr->bufusage.temp_blk_write_time);

        info->wal_records = instr->walusage.wal_records;
        info->wal_fpi = instr->walusage.wal_fpi;
        info->wal_bytes = instr->walusage.wal_bytes;
        info->wal_buffers_full = instr->walusage.wal_buffers_full;
    }else{
        elog(LOG, ">>> totaltime STILL NULL after ExecutorStart");
    }
    if (queryDesc->estate)
    {
        info->rows_processed =
            queryDesc->estate->es_total_processed;
    }
}

static void SetConnectionInfo_End(QueryEndInfo *info)
{
    info->pid = MyProcPid;
    info->backend_start = MyStartTime;

    if (MyProcPort)
    {
        info->database_name = MyProcPort->database_name;
        info->user_name = MyProcPort->user_name;
        info->application_name = MyProcPort->application_name;
        info->remote_host = MyProcPort->remote_host;
        info->remote_port = MyProcPort->remote_port;
    }
}

static void SetQueryInfo_EndHook(QueryEndInfo *info, QueryDesc *queryDesc)
{
    info->query = queryDesc->sourceText;
    info->operation = queryDesc->operation;
    info->params = queryDesc->params;
    info->already_executed = queryDesc->already_executed;
}

static void LogQueryEndInfo(QueryEndInfo *info)
{
    elog(LOG, "========== QUERY END ==========");

    elog(LOG, "PID = %d", info->pid);
    elog(LOG, "DATABASE = %s", info->database_name);
    elog(LOG, "USER = %s", info->user_name);
    elog(LOG, "APPLICATION = %s", info->application_name);
    elog(LOG, "CLIENT HOST = %s", info->remote_host);
    elog(LOG, "CLIENT PORT = %s", info->remote_port);
    elog(LOG, "BACKEND START = %ld", info->backend_start);

    elog(LOG, "QUERY = %s", info->query);
    elog(LOG, "OPERATION = %d", info->operation);
    elog(LOG, "ALREADY EXECUTED = %s", info->already_executed ? "true" : "false");

    elog(LOG, "EXECUTION TIME MS = %f", info->execution_time_ms);
    elog(LOG, "ROWS PROCESSED = " UINT64_FORMAT, info->rows_processed);

    elog(LOG, "SHARED BLKS HIT = " INT64_FORMAT, info->shared_blks_hit);
    elog(LOG, "SHARED BLKS READ = " INT64_FORMAT, info->shared_blks_read);
    elog(LOG, "SHARED BLKS DIRTIED = " INT64_FORMAT, info->shared_blks_dirtied);
    elog(LOG, "SHARED BLKS WRITTEN = " INT64_FORMAT, info->shared_blks_written);

    elog(LOG, "LOCAL BLKS HIT = " INT64_FORMAT, info->local_blks_hit);
    elog(LOG, "LOCAL BLKS READ = " INT64_FORMAT, info->local_blks_read);
    elog(LOG, "LOCAL BLKS DIRTIED = " INT64_FORMAT, info->local_blks_dirtied);
    elog(LOG, "LOCAL BLKS WRITTEN = " INT64_FORMAT, info->local_blks_written);

    elog(LOG, "TEMP BLKS READ = " INT64_FORMAT, info->temp_blks_read);
    elog(LOG, "TEMP BLKS WRITTEN = " INT64_FORMAT, info->temp_blks_written);

    elog(LOG, "SHARED BLK READ TIME MS = %f", info->shared_blk_read_time_ms);
    elog(LOG, "SHARED BLK WRITE TIME MS = %f", info->shared_blk_write_time_ms);
    elog(LOG, "LOCAL BLK READ TIME MS = %f", info->local_blk_read_time_ms);
    elog(LOG, "LOCAL BLK WRITE TIME MS = %f", info->local_blk_write_time_ms);
    elog(LOG, "TEMP BLK READ TIME MS = %f", info->temp_blk_read_time_ms);
    elog(LOG, "TEMP BLK WRITE TIME MS = %f", info->temp_blk_write_time_ms);

    elog(LOG, "WAL RECORDS = " INT64_FORMAT, info->wal_records);
    elog(LOG, "WAL FPI = " INT64_FORMAT, info->wal_fpi);
    elog(LOG, "WAL BYTES = " UINT64_FORMAT, info->wal_bytes);
    elog(LOG, "WAL BUFFERS FULL = " INT64_FORMAT, info->wal_buffers_full);

    elog(LOG, "==============================");
}


static char *QueryEndInfoToJson(QueryEndInfo *info)
{
    StringInfoData json;

    initStringInfo(&json);

    appendStringInfoChar(&json, '{');

    appendStringInfo(&json, "\"operation_name\":");
    escape_json(&json, info->operation_name ? info->operation_name : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"pid\":%d,", info->pid);

    appendStringInfo(&json, "\"database_name\":");
    escape_json(&json, info->database_name ? info->database_name : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"user_name\":");
    escape_json(&json, info->user_name ? info->user_name : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"application_name\":");
    escape_json(&json, info->application_name ? info->application_name : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"remote_host\":");
    escape_json(&json, info->remote_host ? info->remote_host : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"remote_port\":");
    escape_json(&json, info->remote_port ? info->remote_port : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"backend_start\":%ld,", info->backend_start);

    appendStringInfo(&json, "\"query\":");
    escape_json(&json, info->query ? info->query : "");
    appendStringInfoChar(&json, ',');

    appendStringInfo(&json, "\"operation\":%d,", info->operation);
    appendStringInfo(&json, "\"already_executed\":%s,",
                     info->already_executed ? "true" : "false");

    appendStringInfo(&json, "\"execution_time_ms\":%f,", info->execution_time_ms);
    appendStringInfo(&json, "\"rows_processed\":" UINT64_FORMAT ",", info->rows_processed);

    appendStringInfo(&json, "\"shared_blks_hit\":" INT64_FORMAT ",", info->shared_blks_hit);
    appendStringInfo(&json, "\"shared_blks_read\":" INT64_FORMAT ",", info->shared_blks_read);
    appendStringInfo(&json, "\"shared_blks_dirtied\":" INT64_FORMAT ",", info->shared_blks_dirtied);
    appendStringInfo(&json, "\"shared_blks_written\":" INT64_FORMAT ",", info->shared_blks_written);

    appendStringInfo(&json, "\"local_blks_hit\":" INT64_FORMAT ",", info->local_blks_hit);
    appendStringInfo(&json, "\"local_blks_read\":" INT64_FORMAT ",", info->local_blks_read);
    appendStringInfo(&json, "\"local_blks_dirtied\":" INT64_FORMAT ",", info->local_blks_dirtied);
    appendStringInfo(&json, "\"local_blks_written\":" INT64_FORMAT ",", info->local_blks_written);

    appendStringInfo(&json, "\"temp_blks_read\":" INT64_FORMAT ",", info->temp_blks_read);
    appendStringInfo(&json, "\"temp_blks_written\":" INT64_FORMAT ",", info->temp_blks_written);

    appendStringInfo(&json, "\"shared_blk_read_time_ms\":%f,", info->shared_blk_read_time_ms);
    appendStringInfo(&json, "\"shared_blk_write_time_ms\":%f,", info->shared_blk_write_time_ms);
    appendStringInfo(&json, "\"local_blk_read_time_ms\":%f,", info->local_blk_read_time_ms);
    appendStringInfo(&json, "\"local_blk_write_time_ms\":%f,", info->local_blk_write_time_ms);
    appendStringInfo(&json, "\"temp_blk_read_time_ms\":%f,", info->temp_blk_read_time_ms);
    appendStringInfo(&json, "\"temp_blk_write_time_ms\":%f,", info->temp_blk_write_time_ms);

    appendStringInfo(&json, "\"wal_records\":" INT64_FORMAT ",", info->wal_records);
    appendStringInfo(&json, "\"wal_fpi\":" INT64_FORMAT ",", info->wal_fpi);
    appendStringInfo(&json, "\"wal_bytes\":" UINT64_FORMAT ",", info->wal_bytes);
    appendStringInfo(&json, "\"wal_buffers_full\":" INT64_FORMAT ",", info->wal_buffers_full);

    AppendParamsJson(&json, info->params);

    appendStringInfoChar(&json, '}');

    return json.data;
}



static void Extension_executor_end(QueryDesc *queryDesc)
{
    
    
    QueryEndInfo info = {0};

    char *message;

    info.operation_name = "ExecutorEnd";

    SetConnectionInfo_End(&info);
    SetQueryInfo_EndHook(&info, queryDesc);
    SetEndExecutionInfo(&info, queryDesc);

    LogQueryEndInfo(&info);

    message = QueryEndInfoToJson(&info);

    // elog(LOG, "JSON = %s", message);

    HandleJsonMessage(message);

    pfree(message);

    if (prev_ExecutorEnd_hook)
        prev_ExecutorEnd_hook(queryDesc);
    else
        standard_ExecutorEnd(queryDesc);

    
}




#include "executor/spi.h"
#include "postmaster/bgworker.h"
#include "storage/latch.h"
#include "storage/ipc.h"


#include "access/xact.h"
#include "utils/snapmgr.h"
PGDLLEXPORT void DBAnalyzerWorker(Datum main_arg)
{
    int ret;
    int rc;

    elog(LOG, "DB Analyzer Worker: STARTED");

    BackgroundWorkerUnblockSignals();

    BackgroundWorkerInitializeConnection("backendless", NULL, 0);

    for (;;)
    {

        StartTransactionCommand();


        PushActiveSnapshot(GetTransactionSnapshot());


        ret = SPI_connect();


        if (ret != SPI_OK_CONNECT)
        {
            elog(WARNING, "DB Analyzer Worker: SPI_connect failed");

            PopActiveSnapshot();
            AbortCurrentTransaction();

            goto wait_for_next_poll;
        }


        ret = SPI_execute(
            "SELECT json_build_object("
                "'operation_name', 'POLLING', "

                "'GetBasicDbInfo', ("
                    "SELECT json_build_object("
                        "'database_name', current_database(), "
                        "'current_user', current_user, "
                        "'postgres_version', version()"
                    ")"
                "), "

                "'GetUserRolePermissions', ("
                    "SELECT json_build_object("
                        "'rolname', rolname, "
                        "'rolsuper', rolsuper, "
                        "'rolinherit', rolinherit, "
                        "'rolcreaterole', rolcreaterole, "
                        "'rolcreatedb', rolcreatedb, "
                        "'rolcanlogin', rolcanlogin, "
                        "'rolreplication', rolreplication, "
                        "'rolbypassrls', rolbypassrls"
                    ") "
                    "FROM pg_roles "
                    "WHERE rolname = current_user"
                "), "

                "'GetDatabasePermissionsQuery', ("
                    "SELECT json_build_object("
                        "'connect', has_database_privilege(current_user, current_database(), 'CONNECT'), "
                        "'create', has_database_privilege(current_user, current_database(), 'CREATE'), "
                        "'temporary', has_database_privilege(current_user, current_database(), 'TEMPORARY')"
                    ")"
                "), "

                "'GetSchemaLevelInfoQuery', ("
                    "SELECT json_agg("
                        "json_build_object("
                            "'schema_name', nspname, "
                            "'usage', has_schema_privilege(current_user, nspname, 'USAGE'), "
                            "'create', has_schema_privilege(current_user, nspname, 'CREATE')"
                        ")"
                    ") "
                    "FROM pg_namespace"
                "), "

                "'GetTableLevelInfoQuery', ("
                    "SELECT json_agg("
                        "json_build_object("
                            "'schema_name', n.nspname, "
                            "'table_name', c.relname, "
                            "'select', has_table_privilege(current_user, c.oid, 'SELECT'), "
                            "'insert', has_table_privilege(current_user, c.oid, 'INSERT'), "
                            "'update', has_table_privilege(current_user, c.oid, 'UPDATE'), "
                            "'delete', has_table_privilege(current_user, c.oid, 'DELETE'), "
                            "'truncate', has_table_privilege(current_user, c.oid, 'TRUNCATE')"
                        ")"
                    ") "
                    "FROM pg_class c "
                    "JOIN pg_namespace n ON n.oid = c.relnamespace "
                    "WHERE c.relkind IN ('r', 'p')"
                "), "

                "'GetColumnLevelInfoQuery', ("
                    "SELECT json_agg("
                        "json_build_object("
                            "'schema_name', n.nspname, "
                            "'table_name', c.relname, "
                            "'column_name', a.attname, "
                            "'select', has_column_privilege(current_user, c.oid, a.attname, 'SELECT'), "
                            "'insert', has_column_privilege(current_user, c.oid, a.attname, 'INSERT'), "
                            "'update', has_column_privilege(current_user, c.oid, a.attname, 'UPDATE'), "
                            "'references', has_column_privilege(current_user, c.oid, a.attname, 'REFERENCES')"
                        ")"
                    ") "
                    "FROM pg_attribute a "
                    "JOIN pg_class c ON c.oid = a.attrelid "
                    "JOIN pg_namespace n ON n.oid = c.relnamespace "
                    "WHERE c.relkind IN ('r', 'p') "
                    "AND a.attnum > 0 "
                    "AND NOT a.attisdropped"
                "), "

                "'GetConnectionsInfoQuery', ("
                    "SELECT json_agg("
                        "json_build_object("
                            "'pid', pid, "
                            "'username', usename, "
                            "'database', datname, "
                            "'client_addr', client_addr, "
                            "'client_port', client_port, "
                            "'application_name', application_name, "
                            "'state', state, "
                            "'backend_start', backend_start, "
                            "'query_start', query_start, "
                            "'state_change', state_change, "
                            "'wait_event_type', wait_event_type, "
                            "'wait_event', wait_event, "
                            "'query', query"
                        ")"
                    ") "
                    "FROM pg_stat_activity "
                    "WHERE datname IS NOT NULL"
                "), "

                "'GetDatabaseStatsQuery', ("
                    "SELECT json_build_object("
                        "'database', datname, "
                        "'connections', numbackends, "
                        "'xact_commit', xact_commit, "
                        "'xact_rollback', xact_rollback, "
                        "'blks_read', blks_read, "
                        "'blks_hit', blks_hit, "
                        "'tup_returned', tup_returned, "
                        "'tup_fetched', tup_fetched, "
                        "'tup_inserted', tup_inserted, "
                        "'tup_updated', tup_updated, "
                        "'tup_deleted', tup_deleted, "
                        "'temp_files', temp_files, "
                        "'temp_bytes', temp_bytes"
                    ") "
                    "FROM pg_stat_database "
                    "WHERE datname = current_database()"
                "), "

                "'GetLocksInfoQuery', ("
                    "SELECT json_agg("
                        "json_build_object("
                            "'pid', pid, "
                            "'locktype', locktype, "
                            "'database', database, "
                            "'relation', relation, "
                            "'page', page, "
                            "'tuple', tuple, "
                            "'transactionid', transactionid, "
                            "'mode', mode, "
                            "'granted', granted"
                        ")"
                    ") "
                    "FROM pg_locks"
                "), "

                "'GetTableStatsQuery', ("
                    "SELECT json_agg("
                        "json_build_object("
                            "'schemaname', s.schemaname, "
                            "'table_name', s.relname, "
                            "'seq_scan', s.seq_scan, "
                            "'seq_tup_read', s.seq_tup_read, "
                            "'idx_scan', s.idx_scan, "
                            "'idx_tup_fetch', s.idx_tup_fetch, "
                            "'n_tup_ins', s.n_tup_ins, "
                            "'n_tup_upd', s.n_tup_upd, "
                            "'n_tup_del', s.n_tup_del, "
                            "'n_tup_hot_upd', s.n_tup_hot_upd, "
                            "'n_live_tup', s.n_live_tup, "
                            "'n_dead_tup', s.n_dead_tup, "
                            "'last_vacuum', s.last_vacuum, "
                            "'last_autovacuum', s.last_autovacuum, "
                            "'last_analyze', s.last_analyze, "
                            "'last_autoanalyze', s.last_autoanalyze, "
                            "'vacuum_count', s.vacuum_count, "
                            "'autovacuum_count', s.autovacuum_count, "
                            "'analyze_count', s.analyze_count, "
                            "'autoanalyze_count', s.autoanalyze_count, "
                            "'table_size_bytes', pg_table_size(s.relid), "
                            "'index_size_bytes', pg_indexes_size(s.relid), "
                            "'total_size_bytes', pg_total_relation_size(s.relid), "
                            "'heap_blks_read', io.heap_blks_read, "
                            "'heap_blks_hit', io.heap_blks_hit, "
                            "'idx_blks_read', io.idx_blks_read, "
                            "'idx_blks_hit', io.idx_blks_hit, "
                            "'toast_blks_read', io.toast_blks_read, "
                            "'toast_blks_hit', io.toast_blks_hit, "
                            "'tidx_blks_read', io.tidx_blks_read, "
                            "'tidx_blks_hit', io.tidx_blks_hit"
                        ")"
                    ") "
                    "FROM pg_stat_user_tables s "
                    "LEFT JOIN pg_statio_user_tables io ON io.relid = s.relid"
                ")"
            ") AS db_analyzer_snapshot;",
            true,
            0
        );


        if (ret != SPI_OK_SELECT)
        {
            elog(WARNING, "DB Analyzer Worker: SPI_execute failed");
        }
        else if (SPI_processed > 0)
        {
            TupleDesc tupdesc;
            HeapTuple tuple;
            char *json;


            tupdesc = SPI_tuptable->tupdesc;
            tuple = SPI_tuptable->vals[0];


            json = SPI_getvalue(tuple, tupdesc, 1);


            if (json != NULL)
            {
                elog(LOG, "DB Analyzer Worker: JSON length=%zu", strlen(json));
                // elog(LOG, "DB Analyzer Worker: JSON: %s", json);


                HandleJsonMessage(json);
            }
        }

        SPI_finish();


        PopActiveSnapshot();


        CommitTransactionCommand();


wait_for_next_poll:


        rc = WaitLatch(
            MyLatch,
            WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH,
            5000L,
            0
        );

        ResetLatch(MyLatch);


        if (rc & WL_LATCH_SET)
        {
            CHECK_FOR_INTERRUPTS();
        }
    }
}

// #include "access/xact.h"   // Required for StartTransactionCommand / CommitTransactionCommand
// #include "utils/snapmgr.h" // Required for PushActiveSnapshot / PopActiveSnapshot

// PGDLLEXPORT void DBAnalyzerWorker(Datum main_arg)
// {
//     int ret;

//     elog(LOG, "DB Analyzer Worker: STARTED");

//     BackgroundWorkerUnblockSignals();
//     BackgroundWorkerInitializeConnection("backendless", NULL, 0);

//     /* 1. START TRANSACTION */
//     StartTransactionCommand();

//     /* 2. PUSH SNAPSHOT (Crucial for SPI queries) */
//     PushActiveSnapshot(GetTransactionSnapshot());

//     ret = SPI_connect();
//     if (ret != SPI_OK_CONNECT)
//     {
//         elog(WARNING, "DB Analyzer Worker: SPI_connect failed");
//         PopActiveSnapshot();
//         AbortCurrentTransaction(); 
//         return;
//     }

//     elog(LOG, "DB Analyzer Worker: BEFORE SPI_execute");

//     /* 3. EXECUTE YOUR QUERY */
//     ret = SPI_execute("SELECT 1", true, 0);
//     elog(LOG, "DB Analyzer Worker: AFTER SPI_execute, ret=%d", ret);

//     SPI_finish();

//     /* 4. CLEAN UP SNAPSHOT AND COMMIT */
//     PopActiveSnapshot();
//     CommitTransactionCommand();

//     elog(LOG, "DB Analyzer Worker: WORKER FINISHED");
// }


static void CreateBackgroundWorker(void)
{
    BackgroundWorker worker;

    memset(&worker, 0, sizeof(BackgroundWorker));

    snprintf(worker.bgw_name, BGW_MAXLEN, "DB Analyzer Worker");
    snprintf(worker.bgw_type, BGW_MAXLEN, "db_analyzer");

    worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
    worker.bgw_start_time = BgWorkerStart_ConsistentState;
    // worker.bgw_restart_time = 5;

    worker.bgw_restart_time = BGW_NEVER_RESTART;

    strlcpy(worker.bgw_library_name, "extension", MAXPGPATH);
    strlcpy(worker.bgw_function_name, "DBAnalyzerWorker", BGW_MAXLEN);

    RegisterBackgroundWorker(&worker);
}



// Here PostgreSQL will call each library's _PG_init() function and update the ExecutorStart hook.
// So the last extension that is loaded is executed first, and from that function, we move up the chain.
// Here it doesn't call our function; it just registers it.
void _PG_init(void)
{

    socketResult = CreateClientSocket();

    if (socketResult.success)
    {
        elog(LOG, "Extension: connected to Unix socket");
    }
    else
    {
        elog(LOG, "Extension: Unix socket not found");
    }
    
    prev_ExecutorStart_hook = ExecutorStart_hook;
    ExecutorStart_hook = Extension_executor;

    prev_ExecutorEnd_hook = ExecutorEnd_hook;
    ExecutorEnd_hook = Extension_executor_end;

    CreateBackgroundWorker();
}













