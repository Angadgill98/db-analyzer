import express, { type Express } from "express";
import type { Server, Socket } from "socket.io";
import { services } from "./services/services.js";
import { handlers } from "./handlers/handlers.js";
import { Jwt } from "./services/jwt.js";
import { parseCookie, parseSetCookie, type Cookies } from "cookie";
import { queryObjects } from "node:v8";


export class Server_init{

    app:Express
    ws:WebSockets

    constructor(app:Express,io:Server){
        this.app=app;
        this.ws=new WebSockets(io)
    }

    public SetUpWebSockets(){

        // this.ws.io.use((socket,next)=>{
        //     const type = socket.handshake.auth.type;

        //     if (type === "sender") {
        //         socket.data.type = "sender";
        //         return next();
        //     }

        //     let cookieHeader = socket.handshake.headers.cookie;

        //     let cookies = parseCookie(cookieHeader ?? "");

        //     let user_id = this.ws.AuthCookie(cookies);

        //     if (!user_id) {
        //         return next(new Error("Authentication failed"));
        //     }

        //     socket.data.user_id=user_id;
            
        //     next();
        // })
        this.ws.io.on('connect',(socket)=>{
            console.log("asdas")
            
            this.ws.JoinRoomAsClient(socket);

            this.ws.JoinRoomAsSender(socket);

            this.ws.MessageFromSender(socket);
        })
    }

    public SetUpAuthRoutes(){
        let router=express.Router();
        this.app.use('/auth',router);

        router.post('/signup',async(req,res)=>{
            await handlers.auth_handler.SignUp(req)

            
        })

        router.post('/signin',async(req,res)=>{
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
        this.app.use('/api',router)
        router.use(Jwt.AuthMiddleware)
        router.post('/store-db-info',async (req,res)=>{
            await handlers.db_handler.NewUserDb(req);
            
        })

        router.post('/test-connection',async (req,res)=>{
            await handlers.db_handler.TestConnection(req);
        })
 
        router.get('/get-user-dbs',async (req,res)=>{
            await handlers.user_handler.GetUsersDB(req)
        })

        router.post('/get-static',async (req,res)=>{
            await handlers.db_handler.GetStaticData(req);

        })

    }

}


class WebSockets{
    io:Server
    constructor(io:Server){
        this.io=io
    }

    JoinRoomAsClient(socket: Socket) {
        socket.on("join-room", async (body) => {
            let db_name = body.db_name;
            let db_id = body.db_id;
            let user_id = socket.data.user_id;

            let pool_identifier = user_id + "_" + db_name + "_" + db_id;

            let [db, Poolresult] = services.Pool.GetPool(pool_identifier);

            if (!Poolresult || !db) {
                socket.emit("join-room-error", {
                    message: "Database is not registered"
                });

                return;
            }



            let roomid = body.someuserdata + "_" + socket.id;

            socket.data.type = "client";

            socket.join(roomid);

            let join_query=db.CreateJoinQuery(roomid);

            let QueryResult=await db.SendQuery(join_query)

            if (QueryResult.rowCount !== 1) {
                socket.emit("join-room-error", {
                    message: "Failed to register room"
                });

                return;
            }

            socket.emit("join-room-success", {
                room_id: roomid
            });
        });
    }

    JoinRoomAsSender(socket: Socket) {
        socket.on("join-room", (body) => {
            let roomid = body.someuserdata;

            socket.data.type = "sender";
            socket.data.room_id = roomid;

            socket.join(roomid);
        });
    }

    MessageFromSender(socket:Socket){
        socket.on("metrics",(body)=>{
            console.log("asdas")
        })
    }

    

    AuthCookie(cookies: Cookies): string | null {
        let refreshToken = cookies.refresh_token;

        if (!refreshToken) {
            return null;
        }

        let payload = Jwt.VerifyToken(refreshToken);

        if (!payload) {
            return null;
        }

        return payload.user_id;
    }

}