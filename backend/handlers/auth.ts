import type { Request } from "express";
import { services } from "../services/services.js";
import { repos } from "../repos/repos.js";
import { DB } from "../db_init.js";



export class Auth_handler{

    constructor(){

    }


    async SignUp(req:Request){
        let body = req.body;

        let name = body.name;
        let mail = body.mail;
        let pass = body.pass;

        let hash = await services.Auth.HashPassword(pass);

        let [user_id, result] = await repos.User.RegisterUser(DB, mail, hash, name);

        if (!result) {
            return;
        }
    }

    async SignIn(req: Request) {
        let mail=req.body.mail
        let pass=req.body.pass
        try {
            const user = await repos.User.GetUserByMail(DB, mail);

            if (!user) {
                return null;
            }

            const valid = await services.Auth.ComparePassword(pass, user.pass);

            if (!valid) {
                return null;
            }

            return user.id;
        } catch (error) {
            console.error("Operation: SignIn");
            console.error("Parameters:", { mail });
            console.error("Error:", error);

            return null;
        }
    }
}