import bcrypt from "bcrypt";

export class Auth_Service {
    constructor() {

    }

    async HashPassword(password: string): Promise<string> {
        try {
            return await bcrypt.hash(password, 12);
        } catch (error) {
            console.error("Operation: HashPassword");
            console.error("Error:", error);

            throw error;
        }
    }

    async ComparePassword(password: string, hash: string): Promise<boolean> {
        try {
            return await bcrypt.compare(password, hash);
        } catch (error) {
            console.error("Operation: ComparePassword");
            console.error("Error:", error);

            return false;
        }
    }
}