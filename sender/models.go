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