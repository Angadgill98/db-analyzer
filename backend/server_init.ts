import express, { type Express } from "express";
import type { Server, Socket } from "socket.io";
import { services } from "./services/services.js";
import { handlers } from "./handlers/handlers.js";
import { Jwt } from "./services/jwt.js";


export class Server_init{

    app:Express
    ws:WebSockets

    constructor(app:Express,io:Server){
        this.app=app;
        this.ws=new WebSockets(io)
    }

    public SetUpWebSockets(){
        this.ws.io.on('connect',(socket)=>{
            
            this.ws.JoinRoom(socket);
            this.ws.InviteCollector(socket);

        })
    }

    public SetUpAuthRoutes(){
        let router=express.Router();
        this.app.use('/auth',router);

        router.post('signup',async(req,res)=>{
            await handlers.auth_handler.SignUp(req)

            
        })

        router.post('signin',async(req,res)=>{
            let user_id = await handlers.auth_handler.SignIn(req)

            if (!user_id) {
                return res.status(401).json({ message: "Invalid credentials" });
            }

            const accessToken = Jwt.CreateAccessToken(user_id);
            const refreshToken = Jwt.CreateRefreshToken(user_id);

            res.cookie("access_token", accessToken, {
                httpOnly: true,
                secure: process.env.NODE_ENV === "production",
                sameSite: "lax",
                maxAge: 15 * 60 * 1000
            });

            res.cookie("refresh_token", refreshToken, {
                httpOnly: true,
                secure: process.env.NODE_ENV === "production",
                sameSite: "lax",
                maxAge: 7 * 24 * 60 * 60 * 1000
            });
        })
    }
    
    public SetUpApiRoutes(){
        let router=express.Router();
        router.use(Jwt.AuthMiddleware)
        router.post('/store-db-info',async (req,res)=>{
            handlers.db_handler.NewUserDb(req);
            
        })

        router.post('/test-connection',async (req,res)=>{
            handlers.db_handler.TestConnection(req);
        })

        router.get('/get-user-dbs',async (req,res)=>{

        })

        
    }

}


class WebSockets{
    io:Server
    constructor(io:Server){
        this.io=io
    }

    JoinRoom(socket:Socket){
        socket.on('join-room',(body)=>{
            let roomid=body.someuserdata+"_"+socket.id
            socket.join(roomid);
        })
    }    

    InviteCollector(socket:Socket){
        socket.on('invite-collector',async (body)=>{
            let room_id=body.room_id
            let db_iddentifier:string="asdas";
            let [db, ok] = services.Pool.GetPool(db_iddentifier);

            if (!ok) {
                // pool does not exist
                return;
            }
            if (db === undefined) {
                return;
            }

            let query=db.CreateJoinQuery(room_id);
            let result =await db.SendQuery(query);

        })
        
    }
}