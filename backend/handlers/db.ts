import type { Request } from "express";
import { services } from "../services/services.js";
import { repos } from "../repos/repos.js";
import { DB } from "../db_init.js";
import { Pool } from "pg";
import { Client_db } from "../services/db_pool.js";


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

        if (!result) {
            console.error("Operation: NewUserDb - TestConnection failed");

            return {
                status: 400,
                data: {
                    message: "Could not connect to database"
                }
            };
        }

        let [db_cred_id, storeResult] = await repos.Db.StoreDbCredentials(DB,dbName, user_id, credentials.host, credentials.port, credentials.database, credentials.user, credentials.password);

        if (!storeResult || !db_cred_id) {
            console.error("Operation: NewUserDb - StoreDbCredentials failed");

            return {
                status: 500,
                data: {
                    message: "Failed to store database credentials"
                }
            };
        }

        return {
            status: 201,
            data: {
                db_id: db_cred_id,
                db_name: dbName
            }
        };
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

    async RegisterDBinServer(req: Request) {
        try {
            let body = req.body;

            let db_id = body.db_id;
            let db_name = body.db_name;
            let user_id = req.user_id!;

            let credentials = await repos.Db.GetDbCredentials(DB, user_id, db_id);

            if (!credentials) {
                return {
                    status: 404,
                    data: {
                        message: "Database not found"
                    }
                };
            }

            let result = await services.Db.TestConnection(
                credentials.host,
                credentials.port,
                credentials.database_name,
                credentials.username,
                credentials.password
            );

            if (!result) {
                return {
                    status: 400,
                    data: {
                        message: "Could not connect to database"
                    }
                };
            }

            let pool = new Pool({
                host: credentials.host,
                port: credentials.port,
                database: credentials.database_name,
                user: credentials.username,
                password: credentials.password
            });

            let clientDb = new Client_db(pool);

            let pool_identifier = user_id + "_" + db_name + "_" + db_id;

            services.Pool.InsertPool(pool_identifier, clientDb);

            return {
                status: 200,
                data: {
                    message: "Database connected",
                    pool_identifier: pool_identifier
                }
            };
        } catch (error) {
            console.error("Operation: GetStaticData");
            console.error("Error:", error);

            return {
                status: 500,
                data: {
                    message: "Internal server error"
                }
            };
        }
    }

    async GetStaticData(req: Request) {
        try {
            let body = req.body;

            let db_id = body.db_id;
            let db_name = body.db_name;
            let user_id = req.user_id!;

            let pool_identifier = user_id + "_" + db_name + "_" + db_id;

            let [db, result] = services.Pool.GetPool(pool_identifier);

            if (!result || !db) {
                return {
                    status: 404,
                    data: {
                        message: "Database is not registered"
                    }
                };
            }
            if (!db) {
                return {
                    status: 404,
                    data: {
                        message: "Database is not registered"
                    }
                };
            }

            let query = services.Metrics.static.GetStaticDataQuery();

            let queryResult = await db.SendQuery(query);

            let rows = queryResult.rows;

            return {
                status: 200,
                data: rows[0]
            };
        } catch (error) {
            console.error("Operation: GetStaticData");
            console.error("Error:", error);

            return {
                status: 500,
                data: {
                    message: "Internal server error"
                }
            };
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