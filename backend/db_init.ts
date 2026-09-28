import pg from "pg";

const { Pool } = pg;

import dotenv from "dotenv";
dotenv.config();


export const DB = new Pool({
    host: process.env.DB_HOST,
    port: Number(process.env.DB_PORT),
    database: process.env.DB_NAME,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD
});

DB.connect()
    .then((client) => {
        console.log("Connected to database successfully");
        client.release();
    })
    .catch((error) => {
        console.error("Failed to connect to database:", error);
    });