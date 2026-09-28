import { Auth_handler } from "./auth.js";
import { Db_handler } from "./db.js"



class Handlers{
    db_handler:Db_handler;
    auth_handler:Auth_handler
    constructor(){
        this.db_handler=new Db_handler()
        this.auth_handler=new Auth_handler()
    }



}

export const handlers=new Handlers()