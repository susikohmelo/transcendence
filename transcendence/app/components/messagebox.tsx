"use client";

import { useMessage } from "@/context/messagebox";
import { useEffect, useRef, useState } from "react";

export default function MessageDisplay() {
	const { message, messageType } = useMessage();

	const [visible, setVisible] = useState(false);

	const timeoutRef = useRef<NodeJS.Timeout | null>(null);
	const hideTimeoutRef = useRef<NodeJS.Timeout | null>(null);

	useEffect(() => {

		if (!message || !messageType)
			return;

		if (timeoutRef.current) clearTimeout(timeoutRef.current);
		if (hideTimeoutRef.current) clearTimeout(hideTimeoutRef.current);

		setTimeout(() => {
			setVisible(true);
		}, 10);

		timeoutRef.current = setTimeout(() => {
			setVisible(false);
			hideTimeoutRef.current = setTimeout(() => {
			}, 400);
		}, 3600);

	}, [message]);

	return (
		<div className="status-container">
			<div
				className={`status-message ${visible ? "show" : ""} 
					${messageType === "success"
						? "status-success"
						: messageType === "error"
							? "status-error"
							: "status-notice"
					}`}
				aria-hidden={!visible}
			>
				{message}
			</div>
		</div>
	);
}
