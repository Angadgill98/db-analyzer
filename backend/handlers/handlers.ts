import { Auth_handler } from "./auth.js";
import { Db_handler } from "./db.js"
import { User_handler } from "./user.js";



class Handlers{
    db_handler:Db_handler;
    auth_handler:Auth_handler
    user_handler:User_handler
    constructor(){
        this.db_handler=new Db_handler()
        this.auth_handler=new Auth_handler()
        this.user_handler=new User_handler()
    }



}

export const handlers=new Handlers()