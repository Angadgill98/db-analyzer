import type { Pool } from "pg";

export class User_repo {
    async RegisterUser(db: Pool, mail: string, pass: string, name: string): Promise<[string | undefined, boolean]> {
        try {
            const result = await db.query(
                `INSERT INTO users (mail, pass, name, db)
                 VALUES ($1, $2, $3, '{}')
                 RETURNING id`,
                [mail, pass, name]
            );

            if (result.rowCount !== 1) {
                return [undefined, false];
            }

            return [result.rows[0].id, true];
        } catch (error) {
            console.error("Operation: RegisterUser");
            console.error("Parameters:", { mail, name });
            console.error("Error:", error);

            return [undefined, false];
        }
    }

    async RegisterDbID(db: Pool, user_id: string, db_id: string): Promise<boolean> {
        try {
            const result = await db.query(
                `UPDATE users
                 SET db = array_append(db, $1::UUID)
                 WHERE id = $2`,
                [db_id, user_id]
            );

            return result.rowCount === 1;
        } catch (error) {
            console.error("Operation: RegisterDbID");
            console.error("Parameters:", { user_id, db_id });
            console.error("Error:", error);

            return false;
        }
    }

    async RemoveDbID(db: Pool, user_id: string, db_id: string): Promise<boolean> {
        try {
            const result = await db.query(
                `UPDATE users
                 SET db = array_remove(db, $1::UUID)
                 WHERE id = $2`,
                [db_id, user_id]
            );

            return result.rowCount === 1;
        } catch (error) {
            console.error("Operation: RemoveDbID");
            console.error("Parameters:", { user_id, db_id });
            console.error("Error:", error);

            return false;
        }
    }

    async GetUserByMail(db: Pool, mail: string) {
        try {
            const result = await db.query(
                `SELECT id, mail, pass, name, db
                FROM users
                WHERE mail = $1`,
                [mail]
            );

            if (result.rowCount !== 1) {
                return undefined;
            }

            return result.rows[0];
        } catch (error) {
            console.error("Operation: GetUserByMail");
            console.error("Parameters:", { mail });
            console.error("Error:", error);

            return undefined;
        }
    }
}