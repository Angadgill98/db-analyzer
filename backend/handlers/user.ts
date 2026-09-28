import type { Request } from "express";
import { repos } from "../repos/repos.js";
import { DB } from "../db_init.js";

export class User_handler{

    constructor(){

    }

    async GetUsersDB(req: Request) {
        try {
            const user_id = req.user_id!;

            const dbs = await repos.Db.GetUserDbs(DB, user_id);

            if (!dbs) {
                console.error("Operation: GetUsersDB - GetUserDbs failed");

                return {
                    status: 500,
                    data: {
                        message: "Failed to get user databases"
                    }
                };
            }

            return {
                status: 200,
                data: {
                    dbs: dbs
                }
            };
        } catch (error) {
            console.error("Operation: GetUsersDB");
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