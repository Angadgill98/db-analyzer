import type { Pool, QueryResult } from "pg";


export class DbPool{
    Pool:Map<string,Client_db>


    constructor(){
        this.Pool=new Map();

    }


    public GetPool(pool_identifier:string): [Client_db | undefined, boolean]{
        let pool=this.Pool.get(pool_identifier);
        return [pool, pool !== undefined];
    }

    public InsertPool(pool_identifier: string, pool: Client_db): void { this.Pool.set(pool_identifier, pool); } 

    public DeletePool(pool_identifier: string): boolean { return this.Pool.delete(pool_identifier); }


}


export class Client_db{
    Pool:Pool

    constructor(pool:Pool){
        this.Pool=pool;
    }

    public async SendQuery(query: string): Promise<QueryResult> {
        return await this.Pool.query(query);
    }


    public CreateJoinQuery(room_id:string):string {
    return `SELECT backend_command(
        '{
            "operation": "JOIN_ROOM",
            "room_id": "${room_id}"
        }'::jsonb
    );`;
}
}
