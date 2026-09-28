import { Pool } from "pg";



export class Db_services{
    constructor(){

    }

    async TestConnection(host: string, port: number, database: string, user: string, password: string): Promise<boolean> {
        const pool = new Pool({
            host: host,
            port: port,
            database: database,
            user: user,
            password: password
        });

        try {
            await pool.query("SELECT 1");
            return true;
        } catch (error) {
            return false;
        } finally {
            await pool.end();
        }
    }
}