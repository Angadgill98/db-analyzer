import pg from "pg";
import dotenv from "dotenv";

dotenv.config();

const { Pool } = pg;

async function CreateDB(): Promise<pg.Pool> {
    const pool = new Pool({
        host: process.env.DB_HOST,
        port: Number(process.env.DB_PORT),
        database: process.env.DB_NAME,
        user: process.env.DB_USER,
        password: process.env.DB_PASSWORD
    });

    pool.on("error", async (error) => { 
        console.error("Database connection error:", error.message);
        DB = await CreateDB(); 
    });

    while (true) {
        try {
            const client = await pool.connect();

            console.log("Connected to database successfully");

            client.release();

            return pool;
        } catch (error) {
            console.error("Database unavailable, retrying in 2 seconds...");

            await new Promise(resolve => setTimeout(resolve, 2000));
        }
    }
}

export let DB = await CreateDB();