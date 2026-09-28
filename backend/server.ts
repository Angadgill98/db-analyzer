import cors from "cors";
import express, { Router } from "express";
import { createServer } from "node:http";
import { Server } from "socket.io";
import { Server_init } from "./server_init.js";


import dotenv from "dotenv";
// dotenv.config();

const app = express();

const PORT = 3000;

app.use(cors({
    origin: "http://localhost:5173"
}));

app.use(express.json());

const httpServer = createServer(app);

const io = new Server(httpServer, {
  cors: {
    origin: "http://localhost:4200",
    credentials:true
  }
});



let server:Server_init=new Server_init(app,io);

server.SetUpAuthRoutes()
server.SetUpWebSockets()
server.SetUpApiRoutes()

httpServer.listen(PORT, () => {
  console.log("Server running on port 3000");
});




