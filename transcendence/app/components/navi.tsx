"use client";

import Image from "next/image";
import Link from "next/link";
import { useRouter } from "next/navigation";
import { getSession } from "@/lib/session";
import LoginBox from "@/components/loginbox";
import { useState } from "react";

export default function Navi() {
	const router = useRouter();
	const session = getSession();

	const [showLogin, setShowLogin] = useState<boolean>(false);

	return (
		<>
			<nav className="nav">
				<div className="nav-inner">
					<div>
						<Link href="/">
							<Image
								className="nav-logo"
								src="/SuperPongLogo_pixel.png"
								alt="Pong Logo"
								width={220}
								height={170}
							/>
						</Link>
					</div>

					<div className="nav-links">
						<button onClick={() => router.push("/")} className="nav-button">
							Welcome
						</button>
						{session.playerID === -1 || !session.apiToken ? (
							<>
								<button
									onClick={() => setShowLogin(true)}
									className="nav-button"
								>
									Sign In
								</button>
							</>
						) : (
							<>
								<button
									onClick={() => router.push("/quickplay")}
									className="nav-button"
								>
									Quickplay
								</button>
								<button
									onClick={() => router.push("/join")}
									className="nav-button"
								>
									Tournament
								</button>
							</>
						)}
					</div>
				</div>
			</nav>
			{showLogin && <LoginBox onClose={() => setShowLogin(false)} />}
		</>
	);
}
