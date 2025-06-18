"use client";

import React, { createContext, useContext, useState, useRef } from "react";

// export interface MsgContext {
// 	onError: (message: string) => void;
// 	onSuccess: (message: string) => void;
// }

interface MessageContextType {
	showError: (msg: string) => void;
	showSuccess: (msg: string) => void;
	showNotice: (msg: string) => void;
	message: string | null;
	messageType: "success" | "error" | "notice" | null;
}

const MessageContext = createContext<MessageContextType | null>(null);

export const useMessage = () => {
	const context = useContext(MessageContext);
	if (!context) {
		throw new Error("useMessage must be used within a MessageProvider");
	}
	return context;
};

export const MessageProvider: React.FC<{ children: React.ReactNode }> = ({
	children,
}) => {
	const [message, setMessage] = useState<string | null>(null);
	const [messageType, setMessageType] = useState<
		"success" | "error" | "notice" | null
	>(null);
	
	const showMessage = (msg: string, type: "success" | "error" | "notice") => {
		setMessage(msg);
		setMessageType(type);
	};

	// Helper methods
	const showError = (msg: string) => showMessage(msg, "error");
	const showSuccess = (msg: string) => showMessage(msg, "success");
	const showNotice = (msg: string) => showMessage(msg, "notice");

	return (
		<MessageContext.Provider
			value={{
				showError,
				showSuccess,
				showNotice,
				message,
				messageType,
			}}
		>
			{children}
		</MessageContext.Provider>
	);
};
