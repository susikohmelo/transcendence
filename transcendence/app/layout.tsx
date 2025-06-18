import "@/styles/globals.css";
import "@/styles/content.css";
import "@/styles/login.css";
import "@/styles/message.css";
import "@/styles/navi.css";
import "@/styles/create.css";
import "@/styles/room.css";
import "@/styles/gamestats.css";
import "@/styles/alias.css";
import "@/styles/lobby.css";

import React from "react";
import Navi from "@/components/navi";
import Footer from "@/components/footer";
import SessionHandler from "@/context/sessionhandler";
import { MessageProvider } from "./context/messagebox";
import MessageDisplay from "./components/messagebox";

export default function RootLayout({
	children,
}: {
	children: React.ReactNode;
}) {
	return (
		<html lang="en">
			<head>
				<title>Super Pong 3D</title>
				<link 
					rel="icon" 
					type="image/png" 
					href="/favicon.png" />
				<link
					href="https://fonts.googleapis.com/css2?family=Roboto:wght@100;300;400;500;700&display=swap"
					rel="stylesheet"
				/>
				<link
					href="https://fonts.googleapis.com/css2?family=Roboto+Condensed:wght@300;400;700&display=swap"
					rel="stylesheet"
				/>
			</head>
			<body>
				<div className="background-sphere"></div>

				<MessageProvider>
					<MessageDisplay />
					<SessionHandler>
						<Navi></Navi>
						{children}
					</SessionHandler>
					<Footer></Footer>
				</MessageProvider>
			</body>
		</html>
	);
}
