import type { Request } from "express";
import { services } from "../services/services.js";
import { repos } from "../repos/repos.js";
import { DB } from "../db_init.js";


export class Db_handler{
    constructor(){
        
    }

    async NewUserDb(req:Request){
        let body=req.body as NewUserDbDto;

        let dbName = body.db_name;
        let user_id=req.user_id!;
        let credentials = {
            host: body.host,
            port: body.port,
            database: body.database,
            user: body.username,
            password: body.pass
        };

        let result=await services.Db.TestConnection(credentials.host,credentials.port,credentials.database,credentials.user,credentials.password);

        if(!result){

        }

        let [db_cred_id, storeResult] = await repos.Db.StoreDbCredentials(DB,dbName, user_id, credentials.host, credentials.port, credentials.database, credentials.user, credentials.password);

        if (!result) {
            return;
        }
    }


    async TestConnection(req: Request){
        let body=req.body as TestConnectionDto
        let credentials = {
            host: body.host,
            port: body.port,
            database: body.database,
            user: body.username,
            password: body.pass
        };
    
        let result=await services.Db.TestConnection(credentials.host,credentials.port,credentials.database,credentials.user,credentials.password);

        if(!result){

        }
    }
}

interface NewUserDbDto {
    db_name: string;
    host: string;
    port: number;
    database: string;
    username: string;
    pass: string;
}

export interface TestConnectionDto {
    host: string;
    port: number;
    database: string;
    username: string;
    pass: string;
}