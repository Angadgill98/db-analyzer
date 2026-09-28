import type { Pool } from "pg";

export class Db_repo{

    constructor(){

    }

    async StoreDbCredentials(db: Pool, db_name: string, user_id: string, host: string, port: number, database: string, user: string, password: string): Promise<[string | undefined, boolean]> {
        try {
            const result = await db.query(
                `INSERT INTO db_credentials (db_name, user_id, host, port, database_name, username, password)
                VALUES ($1, $2, $3, $4, $5, $6, $7)
                RETURNING id`,
                [db_name, user_id, host, port, database, user, password]
            );

            if (result.rowCount !== 1) {
                return [undefined, false];
            }

            return [result.rows[0].id, true];
        } catch (error) {
            console.error("Operation: StoreDbCredentials");
            console.error("Parameters:", {
                db_name,
                user_id,
                host,
                port,
                database,
                user,
                password
            });
            console.error("Error:", error);

            return [undefined, false];
        }
    }

    async GetUserDbs(db: Pool, user_id: string) {
        const result = await db.query(
            `SELECT id, db_name
            FROM db_credentials
            WHERE user_id = $1`,
            [user_id]
        );

        return result.rows;
    }

    async GetDbCredentials(db: Pool, user_id: string, db_id: string) {
        const result = await db.query(
            `SELECT id, db_name, host, port, database_name, username, password
            FROM db_credentials
            WHERE user_id = $1 AND id = $2`,
            [user_id, db_id]
        );

        return result.rows;
    }
}