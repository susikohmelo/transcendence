"use client";

import React, { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import { sendMsg, setPlayerName } from "@/lib/session";
import { useMessage } from "@/context/messagebox";

export default function Join() {
	const router = useRouter();
	const { showError } = useMessage();

	const defaultName = "Anonymous";
	const [alias, setAlias] = useState<string>(defaultName);
	const [reset, setReset] = useState<boolean>(false);

	function handleJoin() {

		const trimmed = alias.trim();

		if (alias.length > 0 && trimmed.length === 0) {
			showError("Alias cannot be empty.");
			return;
		}

		if (alias != trimmed) {
			showError('Alias cannot start or end with white space');
			return;
		}

		setPlayerName(alias || defaultName);

		// test SQLite db
		// sendMsg({ type: 'register', username: alias, password: 'test123', passconf: 'test123' });
		// sendMsg({ type: 'sign-in', username: alias, password: 'test123' });

		router.push("/tournament");
	}

	return (
		<main>
			<div className="content">
				<div className="content-line"></div>

				<div className="content-body">
					<h2 className="content-header">Join Tournament</h2>

					<div style={{ height: "6rem" }}></div>

					<div className="alias-row">
						<div className="alias-label">Alias:</div>

						<input
							className="alias-input"
							type="text"
							placeholder={defaultName}
							value={alias}
							maxLength={25}
							onChange={(e) => {
								setAlias(e.target.value);
							}}
							onClick={() => {
								if (!reset && alias === defaultName) {
									setAlias("");
									setReset(true);
								}
							}}
						/>
					</div>

					{/* {error && <div className="alias-error">{error}</div>} */}

					<div style={{ height: "1rem" }}></div>

					<div className="join-button-wrapper">
						<button className="join-button" onClick={handleJoin}>
							JOIN
						</button>
					</div>
				</div>
			</div>
		</main>
	);
}
