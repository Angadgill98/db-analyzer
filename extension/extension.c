//coontrol fiel is liek metadat fo the extesion as toinformation about teh extension 
//in make file module is the c file anem extsionwaht to callt hsi extensionad da t is slq file anme
#include "postgres.h"
#include "executor/executor.h"
#include "fmgr.h"
#include "miscadmin.h"

PG_MODULE_MAGIC;


// This is used to store the previous hook of some other library.
// In PostgreSQL, each extension can have its own hook. To make sure each
// extension's code executes, we store the function pointer and then use it
// to maintain the hook chain.

// This stores the previous ExecutorStart hook.

static ExecutorStart_hook_type prev_ExecutorStart_hook = NULL;
// ExecutorStart_hook_type is a typedef (alias) for a function-pointer type.
// It represents a function that returns void and takes:
// (QueryDesc *, int) as params
//
// Conceptually:
// void (*function_pointer)(QueryDesc *, int)
//
// So prev_ExecutorStart_hook can point to a function
// with exactly this signature.




static char socketPath[] = "/tmp/my_socket";

#include "libpq/libpq-be.h"
static void Extension_executor(QueryDesc *queryDesc, int eflags)
{
    
    elog(LOG, "MY EXTENSION: PID = %d", MyProcPid);

    if (MyProcPort)
    {
        elog(LOG, "DATABASE = %s", MyProcPort->database_name);
        elog(LOG, "USER = %s", MyProcPort->user_name);
        elog(LOG, "APPLICATION = %s", MyProcPort->application_name);
        elog(LOG, "CLIENT HOST = %s", MyProcPort->remote_host);
        elog(LOG, "CLIENT PORT = %s", MyProcPort->remote_port);
        elog(LOG, "BACKEND START / WHEN CLIENT CONNECTED = %ld", MyStartTime);
    }

    elog(LOG, "QUERY = %s", queryDesc->sourceText);


    elog(LOG, "MY EXTENSION: QUERY = %s", queryDesc->sourceText);










    if (prev_ExecutorStart_hook)
        prev_ExecutorStart_hook(queryDesc, eflags);
    else
        standard_ExecutorStart(queryDesc, eflags);
}




// Here PostgreSQL will call each library's _PG_init() function and update the ExecutorStart hook.
// So the last extension that is loaded is executed first, and from that function, we move up the chain.
// Here it doesn't call our function; it just registers it.
void _PG_init(void)
{
    prev_ExecutorStart_hook = ExecutorStart_hook;
    InitailizeCollector(PostmasterPid);
    ExecutorStart_hook = Extension_executor;

}


void InitailizeCollector(pid_t pg_pid){
    elog(LOG,"postgress application is running on pid:%d",pg_pid);
    
}




#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

typedef struct {
    bool success;
    int socket_fd;
    char message[256];
} SocketResult;

SocketResult CreateClientSocket() {
    SocketResult result = {
        .success = false,
        .socket_fd = -1
    };

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        elog(WARNING, "Extension: failed to create socket: %m");
        return result;
    }

    struct sockaddr_un addr;
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











