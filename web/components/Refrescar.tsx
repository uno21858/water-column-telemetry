"use client";

import { useEffect } from "react";
import { useRouter } from "next/navigation";

// Vuelve a pedir la página al servidor cada cierto tiempo, como la sala de control en vivo
export function Refrescar({ cadaMs }: { cadaMs: number }) {
    const router = useRouter();

    useEffect(() => {
        const id = setInterval(() => router.refresh(), cadaMs);
        return () => clearInterval(id);
    }, [router, cadaMs]);

    return null;
}
