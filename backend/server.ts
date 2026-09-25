import cors from "cors";
import express from "express";
import { createServer } from "node:http";

const app = express();

const PORT = 3000;


app.use(cors({
    origin: "http://localhost:5173"
}));

app.use(express.json());

app.get("/api/health", (req, res) => {
    res.json({
        status: "ok"
    });
});


import { Pool } from "pg";
import { Server } from "socket.io";

let pool=new Map<string,Pool>();

app.post("/register-connection",(req,res)=>{
    let body=req.body as Register_Connection_dto
    let connection_pool= new Pool({
        host: body.host,
        port: body.port,
        database: body.database,
        user: body.username,
        password: body.pass
    })

    pool.set("user_id_"+body.database,connection_pool)

})



interface Register_Connection_dto{
    host:string,
    port:number,
    username:string,
    pass:string,
    database:string
}


const httpServer = createServer(app);

const io = new Server(httpServer, {
  cors: {
    origin: "http://localhost:4200"
  }
});

io.on("connection", (socket) => {
    console.log("Client connected:", socket.id);


    socket.on("db-wide-metrics",(db_name,user_id)=>{
        socket.data.db_connection= pool.get(user_id+db_name)
        let db=socket.data.db.connection_pool;

    });

    socket.on("disconnect", () => {
        console.log("Client disconnected:", socket.id);
    });
});

async function GetBasicDbInfo(db:Pool){
    const result = await db.query(`
        SELECT
            current_database() AS database_name,
            current_user AS current_user,
            version() AS postgres_version;
    `);

    return result.rows[0];
}

async function GetUserRolePermissions(db: Pool) {
    const result = await db.query(`
        SELECT
            rolname,
            rolsuper,
            rolinherit,
            rolcreaterole,
            rolcreatedb,
            rolcanlogin,
            rolreplication,
            rolbypassrls
        FROM pg_roles
        WHERE rolname = current_user;
    `);

    return result.rows[0];
}

function GetDatabasePermissionsQuery() {
    return `
        SELECT json_build_object(
            'connect', has_database_privilege(current_user, current_database(), 'CONNECT'),
            'create', has_database_privilege(current_user, current_database(), 'CREATE'),
            'temporary', has_database_privilege(current_user, current_database(), 'TEMPORARY')
        )
    `;
}

function GetSchemaLevelInfoQuery() {
    return `
        SELECT json_agg(
            json_build_object(
                'schema_name', nspname,
                'usage', has_schema_privilege(current_user, nspname, 'USAGE'),
                'create', has_schema_privilege(current_user, nspname, 'CREATE')
            )
        )
        FROM pg_namespace
    `;
}

function GetQueryBuilder(){
    let base_query="SELECT "
    return base_query;
}

function GetTableLevelInfoQuery() {
    return `
        SELECT json_agg(
            json_build_object(
                'schema_name', n.nspname,
                'table_name', c.relname,
                'select', has_table_privilege(current_user, c.oid, 'SELECT'),
                'insert', has_table_privilege(current_user, c.oid, 'INSERT'),
                'update', has_table_privilege(current_user, c.oid, 'UPDATE'),
                'delete', has_table_privilege(current_user, c.oid, 'DELETE'),
                'truncate', has_table_privilege(current_user, c.oid, 'TRUNCATE')
            )
        )
        FROM pg_class c
        JOIN pg_namespace n ON n.oid = c.relnamespace
        WHERE c.relkind IN ('r', 'p')
    `;
}

function GetColumnLevelInfoQuery() {
    return `
        SELECT json_agg(
            json_build_object(
                'schema_name', n.nspname,
                'table_name', c.relname,
                'column_name', a.attname,
                'select', has_column_privilege(current_user, c.oid, a.attname, 'SELECT'),
                'insert', has_column_privilege(current_user, c.oid, a.attname, 'INSERT'),
                'update', has_column_privilege(current_user, c.oid, a.attname, 'UPDATE'),
                'references', has_column_privilege(current_user, c.oid, a.attname, 'REFERENCES')
            )
        )
        FROM pg_attribute a
        JOIN pg_class c ON c.oid = a.attrelid
        JOIN pg_namespace n ON n.oid = c.relnamespace
        WHERE c.relkind IN ('r', 'p')
          AND a.attnum > 0
          AND NOT a.attisdropped
    `;
}

function AddSubQuery(builder_base_query:string,query:string,alias_name:string){
    let separator = builder_base_query.trimEnd().endsWith("SELECT") ? "" : ", ";

    return `${builder_base_query}${separator}(${query}) AS ${alias_name}`;
}








//contnuous data 
function GetConnectionsInfoQuery() {
    return `
        SELECT
            pid,
            usename AS username,
            datname AS database,
            client_addr,
            client_port,
            application_name,
            state,
            backend_start,
            query_start,
            state_change,
            wait_event_type,
            wait_event,
            query
        FROM pg_stat_activity
        WHERE datname IS NOT NULL;
    `;
}


function GetDatabaseStatsQuery() {
    return `
        SELECT
            datname AS database,
            numbackends AS connections,
            xact_commit,
            xact_rollback,
            blks_read,
            blks_hit,
            tup_returned,
            tup_fetched,
            tup_inserted,
            tup_updated,
            tup_deleted,
            temp_files,
            temp_bytes
        FROM pg_stat_database
        WHERE datname = current_database();
    `;
}

function GetLocksInfoQuery() {
    return `
        SELECT
            pid,
            locktype,
            database,
            relation,
            page,
            tuple,
            transactionid,
            mode,
            granted
        FROM pg_locks;
    `;
}

function GetTableStatsQuery() {
    return `
        SELECT
            s.schemaname,
            s.relname AS table_name,

            -- Activity
            s.seq_scan,
            s.seq_tup_read,
            s.idx_scan,
            s.idx_tup_fetch,

            -- Row changes
            s.n_tup_ins,
            s.n_tup_upd,
            s.n_tup_del,
            s.n_tup_hot_upd,

            -- Row estimates
            s.n_live_tup,
            s.n_dead_tup,

            -- Maintenance
            s.last_vacuum,
            s.last_autovacuum,
            s.last_analyze,
            s.last_autoanalyze,
            s.vacuum_count,
            s.autovacuum_count,
            s.analyze_count,
            s.autoanalyze_count,

            -- Storage
            pg_table_size(s.relid) AS table_size_bytes,
            pg_indexes_size(s.relid) AS index_size_bytes,
            pg_total_relation_size(s.relid) AS total_size_bytes,

            -- I/O
            io.heap_blks_read,
            io.heap_blks_hit,
            io.idx_blks_read,
            io.idx_blks_hit,
            io.toast_blks_read,
            io.toast_blks_hit,
            io.tidx_blks_read,
            io.tidx_blks_hit

        FROM pg_stat_user_tables s
        LEFT JOIN pg_statio_user_tables io
            ON io.relid = s.relid;
    `;
}

httpServer.listen(3000, () => {
  console.log("Server running on port 3000");
});