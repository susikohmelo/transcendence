"use client";

import React from "react";
import Link from "next/link";

export default function Home() {
	return (
		<main>
			<div className="content">
				<div className="content-line"></div>

				<div className="content-body-welcome">
					<h2 className="content-header">Welcome to Super Pong 3D!</h2>

					<div className="content-columns">
						<div className="column">
							<h3 className="con-title">
								JUMP TO
								<br />
								THE ACTION
							</h3>

							<Link href="/quickplay">
								<button className="cta-button">
									<span className="con-button">QUICKPLAY</span>
									<img src="/arrow.svg" alt="Arrow" className="arrow-icon" />
								</button>
							</Link>
						</div>

						<div className="column">
							<h3 className="con-title">
								MULTIPLAYER
								<br />
								EXPERIENCE
							</h3>
							<Link href="/join">
								<button className="cta-button">
									<span className="con-button">TOURNAMENT</span>
									<img src="/arrow.svg" alt="Arrow" className="arrow-icon" />
								</button>
							</Link>
						</div>
					</div>
				</div>
			</div>
		</main>
	);
}
