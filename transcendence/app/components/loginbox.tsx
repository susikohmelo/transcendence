"use client";

import { useMessage } from "@/context/messagebox";
import { getSession, sendMsg } from "@/lib/session";
import { useEffect, useRef, useState } from "react";

export default function LoginBox({ onClose }: { onClose: () => void }) {
	const [loading, setLoading] = useState<boolean>(false);
	const [mode, setMode] = useState<"sign-in" | "register">("sign-in");

	const { showError } = useMessage();
	const session = getSession();

	const formRef = useRef<HTMLDivElement>(null);

	useEffect(() => {
		function handleClickOutside(event: MouseEvent) {
			if (formRef.current && !formRef.current.contains(event.target as Node)) {
				onClose();
			}
		}

		document.addEventListener("mousedown", handleClickOutside);

		return () => {
			document.removeEventListener("mousedown", handleClickOutside);
		};
	}, [onClose]);

	useEffect(() => {
		if (session.apiToken) onClose();
	}, [session.apiToken]);

	const handleSignIn = (e: React.FormEvent<HTMLFormElement>) => {
		e.preventDefault();
		setLoading(true);

		const formData = new FormData(e.currentTarget);
		const username = formData.get("username") as string;
		const password = formData.get("password") as string;

		sendMsg({
			type: "sign-in",
			username,
			password,
		});
	};

	const handleRegister = (e: React.FormEvent<HTMLFormElement>) => {
		e.preventDefault();
		setLoading(true);

		const formData = new FormData(e.currentTarget);
		const username = formData.get("username") as string;
		const password = formData.get("password") as string;
		const passconf = formData.get("passconf") as string;

		if (password != passconf) {
			showError("passwords must match");
			return;
		}

		sendMsg({
			type: "register",
			username,
			password,
			passconf,
		});
	};

	function SignInForm() {
		return (
			<form onSubmit={handleSignIn}>
				<ul>
					<li>
						<strong>Sign In</strong>
					</li>
					<li onClick={() => setMode("register")}>Register</li>
				</ul>
				<label htmlFor="username">Username</label>
				<input id="username" name="username" maxLength={30} />
				<label htmlFor="password">Password</label>
				<input id="password" name="password" maxLength={30} type="password" />
				<button>Sign In</button>
			</form>
		);
	}

	function RegisterForm() {
		return (
			<form onSubmit={handleRegister}>
				<ul>
					<li onClick={() => setMode("sign-in")}>Sign In</li>
					<li>
						<strong>Register</strong>
					</li>
				</ul>
				<label htmlFor="username">Username</label>
				<input id="username" name="username" maxLength={30} />
				<label htmlFor="password">Password</label>
				<input id="password" name="password" maxLength={30} type="password" />
				<label htmlFor="passconf">Confirm Password</label>
				<input id="passconf" name="passconf" maxLength={30} type="password" />
				<button>Register</button>
			</form>
		);
	}

	return (
		<div className="login-overlay" onClick={() => console.log("click")}>
			<div className="login-box" ref={formRef}>
				{mode === "sign-in" ? <SignInForm /> : <RegisterForm />}
			</div>
		</div>
	);
}
