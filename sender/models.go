package main;



type OperationHeader struct {
	OperationName string `json:"operation_name"`
}

type OperationType string

const (
	ExecutorStart  OperationType = "ExecutorStart"
	ExecutorEnd    OperationType = "ExecutorEnd"
	BackendCommand OperationType = "BackendCommand"
)

type ExecutorStartOperation struct {
	OperationName   string  `json:"operation_name"`
	PID             int     `json:"pid"`
	DatabaseName    string  `json:"database_name"`
	UserName        string  `json:"user_name"`
	ApplicationName string  `json:"application_name"`
	RemoteHost      string  `json:"remote_host"`
	RemotePort      string  `json:"remote_port"`
	BackendStart    int64   `json:"backend_start"`
	Query           string  `json:"query"`
	Operation       int     `json:"operation"`
	AlreadyExecuted bool    `json:"already_executed"`
	Params          []Param `json:"params"`
}

type Param struct {
	ParamID int    `json:"param_id"`
	TypeOID uint32 `json:"type_oid"`
	IsNull  bool   `json:"is_null"`
	Value   any    `json:"value"`
}

type ExecutorEndOperation struct {
	OperationName          string  `json:"operation_name"`
	PID                    int     `json:"pid"`
	DatabaseName           string  `json:"database_name"`
	UserName               string  `json:"user_name"`
	ApplicationName        string  `json:"application_name"`
	RemoteHost             string  `json:"remote_host"`
	RemotePort             string  `json:"remote_port"`
	BackendStart           int64   `json:"backend_start"`
	Query                  string  `json:"query"`
	Operation              int     `json:"operation"`
	AlreadyExecuted        bool    `json:"already_executed"`
	ExecutionTimeMS        float64 `json:"execution_time_ms"`
	RowsProcessed          uint64  `json:"rows_processed"`
	SharedBlksHit          int64   `json:"shared_blks_hit"`
	SharedBlksRead         int64   `json:"shared_blks_read"`
	SharedBlksDirtied      int64   `json:"shared_blks_dirtied"`
	SharedBlksWritten      int64   `json:"shared_blks_written"`
	LocalBlksHit           int64   `json:"local_blks_hit"`
	LocalBlksRead           int64   `json:"local_blks_read"`
	LocalBlksDirtied       int64   `json:"local_blks_dirtied"`
	LocalBlksWritten       int64   `json:"local_blks_written"`
	TempBlksRead           int64   `json:"temp_blks_read"`
	TempBlksWritten        int64   `json:"temp_blks_written"`
	SharedBlkReadTimeMS    float64 `json:"shared_blk_read_time_ms"`
	SharedBlkWriteTimeMS   float64 `json:"shared_blk_write_time_ms"`
	LocalBlkReadTimeMS     float64 `json:"local_blk_read_time_ms"`
	LocalBlkWriteTimeMS    float64 `json:"local_blk_write_time_ms"`
	TempBlkReadTimeMS      float64 `json:"temp_blk_read_time_ms"`
	TempBlkWriteTimeMS     float64 `json:"temp_blk_write_time_ms"`
	WALRecords             int64   `json:"wal_records"`
	WALFPI                 int64   `json:"wal_fpi"`
	WALBytes               uint64  `json:"wal_bytes"`
	WALBuffersFull         int64   `json:"wal_buffers_full"`
	Params                 []Param `json:"params"`
}

type BackendCommandOperation struct {
	OperationName string `json:"operation_name"`
	RoomID        string `json:"room_id"`
}














type PollingOperation struct {
    OperationName               string                       `json:"operation_name"`
    GetBasicDbInfo              BasicDbInfo                  `json:"GetBasicDbInfo"`
    GetUserRolePermissions      UserRolePermissions           `json:"GetUserRolePermissions"`
    GetDatabasePermissionsQuery DatabasePermissions           `json:"GetDatabasePermissionsQuery"`
    GetSchemaLevelInfoQuery     []SchemaLevelInfo             `json:"GetSchemaLevelInfoQuery"`
    GetTableLevelInfoQuery      []TableLevelInfo              `json:"GetTableLevelInfoQuery"`
    GetColumnLevelInfoQuery     []ColumnLevelInfo             `json:"GetColumnLevelInfoQuery"`
    GetConnectionsInfoQuery     []ConnectionInfo              `json:"GetConnectionsInfoQuery"`
    GetDatabaseStatsQuery       DatabaseStats                `json:"GetDatabaseStatsQuery"`
    GetLocksInfoQuery            []LockInfo                    `json:"GetLocksInfoQuery"`
    GetTableStatsQuery           []TableStats                  `json:"GetTableStatsQuery"`
}

type BasicDbInfo struct {
    DatabaseName   string `json:"database_name"`
    CurrentUser    string `json:"current_user"`
    PostgresVersion string `json:"postgres_version"`
}

type UserRolePermissions struct {
    Rolname        string `json:"rolname"`
    RolSuper       bool   `json:"rolsuper"`
    RolInherit     bool   `json:"rolinherit"`
    RolCreateRole  bool   `json:"rolcreaterole"`
    RolCreateDB    bool   `json:"rolcreatedb"`
    RolCanLogin    bool   `json:"rolcanlogin"`
    RolReplication bool   `json:"rolreplication"`
    RolBypassRLS   bool   `json:"rolbypassrls"`
}

type DatabasePermissions struct {
    Connect   bool `json:"connect"`
    Create    bool `json:"create"`
    Temporary bool `json:"temporary"`
}

type SchemaLevelInfo struct {
    SchemaName string `json:"schema_name"`
    Usage      bool   `json:"usage"`
    Create     bool   `json:"create"`
}

type TableLevelInfo struct {
    SchemaName string `json:"schema_name"`
    TableName  string `json:"table_name"`
    Select     bool   `json:"select"`
    Insert     bool   `json:"insert"`
    Update     bool   `json:"update"`
    Delete     bool   `json:"delete"`
    Truncate   bool   `json:"truncate"`
}

type ColumnLevelInfo struct {
    SchemaName string `json:"schema_name"`
    TableName  string `json:"table_name"`
    ColumnName string `json:"column_name"`
    Select     bool   `json:"select"`
    Insert     bool   `json:"insert"`
    Update     bool   `json:"update"`
    References bool   `json:"references"`
}

type ConnectionInfo struct {
    PID            int        `json:"pid"`
    Username       string     `json:"username"`
    Database       string     `json:"database"`
    ClientAddr     *string    `json:"client_addr"`
    ClientPort     *int       `json:"client_port"`
    ApplicationName string    `json:"application_name"`
    State          *string    `json:"state"`
    BackendStart   string     `json:"backend_start"`
    QueryStart     string     `json:"query_start"`
    StateChange    string     `json:"state_change"`
    WaitEventType  *string    `json:"wait_event_type"`
    WaitEvent      *string    `json:"wait_event"`
    Query          string     `json:"query"`
}

type DatabaseStats struct {
    Database     string `json:"database"`
    Connections  int64  `json:"connections"`
    XactCommit   int64  `json:"xact_commit"`
    XactRollback int64  `json:"xact_rollback"`
    BlksRead     int64  `json:"blks_read"`
    BlksHit      int64  `json:"blks_hit"`
    TupReturned  int64  `json:"tup_returned"`
    TupFetched   int64  `json:"tup_fetched"`
    TupInserted  int64  `json:"tup_inserted"`
    TupUpdated   int64  `json:"tup_updated"`
    TupDeleted   int64  `json:"tup_deleted"`
    TempFiles    int64  `json:"temp_files"`
    TempBytes    int64  `json:"temp_bytes"`
}

type LockInfo struct {
    PID           int     `json:"pid"`
    LockType      string  `json:"locktype"`
    Database      *string `json:"database"`
    Relation      *string `json:"relation"`
    Page          *int64  `json:"page"`
    Tuple         *int64  `json:"tuple"`
    TransactionID *string `json:"transactionid"`
    Mode          string  `json:"mode"`
    Granted       bool    `json:"granted"`
}

type TableStats struct {
    SchemaName       string  `json:"schemaname"`
    TableName        string  `json:"table_name"`
    SeqScan          int64   `json:"seq_scan"`
    SeqTupRead       int64   `json:"seq_tup_read"`
    IdxScan          *int64  `json:"idx_scan"`
    IdxTupFetch      *int64  `json:"idx_tup_fetch"`
    NTupIns          int64   `json:"n_tup_ins"`
    NTupUpd          int64   `json:"n_tup_upd"`
    NTupDel          int64   `json:"n_tup_del"`
    NTupHotUpd       int64   `json:"n_tup_hot_upd"`
    NLiveTup         int64   `json:"n_live_tup"`
    NDeadTup         int64   `json:"n_dead_tup"`
    LastVacuum       *string `json:"last_vacuum"`
    LastAutovacuum   *string `json:"last_autovacuum"`
    LastAnalyze      *string `json:"last_analyze"`
    LastAutoanalyze  *string `json:"last_autoanalyze"`
    VacuumCount      int64   `json:"vacuum_count"`
    AutovacuumCount  int64   `json:"autovacuum_count"`
    AnalyzeCount     int64   `json:"analyze_count"`
    AutoanalyzeCount int64   `json:"autoanalyze_count"`
    TableSizeBytes   int64   `json:"table_size_bytes"`
    IndexSizeBytes   int64   `json:"index_size_bytes"`
    TotalSizeBytes   int64   `json:"total_size_bytes"`
    HeapBlksRead     *int64  `json:"heap_blks_read"`
    HeapBlksHit      *int64  `json:"heap_blks_hit"`
    IdxBlksRead      *int64  `json:"idx_blks_read"`
    IdxBlksHit       *int64  `json:"idx_blks_hit"`
    ToastBlksRead    *int64  `json:"toast_blks_read"`
    ToastBlksHit     *int64  `json:"toast_blks_hit"`
    TidxBlksRead     *int64  `json:"tidx_blks_read"`
    TidxBlksHit      *int64  `json:"tidx_blks_hit"`
}