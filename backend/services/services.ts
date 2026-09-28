import { Auth_Service } from "./auth.js";
import { Db_services } from "./db.js";
import { DbPool } from "./db_pool.js";


class Services{
    Pool:DbPool;
    Db:Db_services;
    Auth:Auth_Service
    constructor(){
        this.Pool=new DbPool();
        this.Db=new Db_services();
        this.Auth=new Auth_Service();
    }
}


export const services:Services=new Services();