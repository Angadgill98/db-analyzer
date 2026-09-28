import type { Request, Response, NextFunction } from "express";
import jwt from "jsonwebtoken";

declare global {
    namespace Express {
        interface Request {
            user_id?: string;
        }
    }
}

class JwtService {
    private secret: string;

    constructor() {
        this.secret = process.env.JWT_SECRET!;

        if (!this.secret) {
            throw new Error("JWT_SECRET is not configured");
        }
    }

    CreateAccessToken(user_id: string): string {
        return jwt.sign(
            { user_id: user_id },
            this.secret,
            { expiresIn: "15m" }
        );
    }

    CreateRefreshToken(user_id: string): string {
        return jwt.sign(
            { user_id: user_id },
            this.secret,
            { expiresIn: "7d" }
        );
    }

    VerifyToken(token: string): { user_id: string } | null {
        try {
            const payload = jwt.verify(token, this.secret);

            if (typeof payload === "string") {
                return null;
            }

            if (typeof payload.user_id !== "string") {
                return null;
            }

            return {
                user_id: payload.user_id
            };
        } catch {
            return null;
        }
    }

    AuthMiddleware(req: Request, res: Response, next: NextFunction) {
        const token = req.cookies.access_token;

        if (!token) {
            return res.status(401).json({
                message: "Access token missing"
            });
        }

        const payload = this.VerifyToken(token);

        if (!payload) {
            return res.status(401).json({
                message: "Invalid or expired access token"
            });
        }

        req.user_id = payload.user_id;

        next();
    }
}


export const Jwt=new JwtService();