import { Db_repo } from "./db.js";
import { User_repo } from "./user.js";


class Repos{
    User:User_repo
    Db:Db_repo
    constructor(){
        this.User=new User_repo;
        this.Db=new Db_repo;
    }
}


export const repos:Repos=new Repos();