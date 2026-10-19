"use client";

import { useEffect, useRef, useSyncExternalStore, useTransition } from "react";
import { useRouter } from "next/navigation";

const OPCIONES = [
    { ms: 10000, texto: "10 s" },
    { ms: 30000, texto: "30 s" },
    { ms: 60000, texto: "1 min" },
    { ms: 300000, texto: "5 min" },
    { ms: 0, texto: "Pausado" },
];
const POR_DEFECTO = 10000;
const LLAVE = "autorefresh-ms";

// La elección se guarda en el navegador de cada quien. Si no hay localStorage, se usa el default
const avisos = new Set<() => void>();

function leer(): number {
    try {
        const guardado = localStorage.getItem(LLAVE);
        const v = Number(guardado);
        return guardado !== null && OPCIONES.some((o) => o.ms === v) ? v : POR_DEFECTO;
    } catch {
        return POR_DEFECTO;
    }
}

function guardar(ms: number) {
    try {
        localStorage.setItem(LLAVE, String(ms));
    } catch {
        // sin almacenamiento: solo dura mientras la pestaña esté abierta
    }
    avisos.forEach((f) => f());
}

function suscribir(f: () => void) {
    avisos.add(f);
    return () => avisos.delete(f);
}

// Vuelve a pedir la página al servidor cada cierto tiempo, como la sala de control en vivo.
// La placa sube cada 30 s, así que menos de 10 s no trae nada nuevo
export function Refrescar() {
    const router = useRouter();
    const cadaMs = useSyncExternalStore(suscribir, leer, () => POR_DEFECTO);
    const [cargando, startTransition] = useTransition();
    const cargandoRef = useRef(false);

    useEffect(() => {
        cargandoRef.current = cargando;
    }, [cargando]);

    useEffect(() => {
        if (cadaMs === 0) {
            return;
        }
        const refrescar = () => {
            // Si la consulta anterior no ha terminado, no se encima otra
            if (!cargandoRef.current) {
                startTransition(() => router.refresh());
            }
        };
        // Con la pestaña oculta no se consulta nada; al volver se pone al día de inmediato
        const id = setInterval(() => {
            if (!document.hidden) {
                refrescar();
            }
        }, cadaMs);
        const alVolver = () => {
            if (!document.hidden) {
                refrescar();
            }
        };
        document.addEventListener("visibilitychange", alVolver);
        return () => {
            clearInterval(id);
            document.removeEventListener("visibilitychange", alVolver);
        };
    }, [router, cadaMs]);

    return (
        <label className="flex items-center gap-2 text-sm text-[#969696]">
            Actualizar cada
            <select
                value={cadaMs}
                onChange={(e) => guardar(Number(e.target.value))}
                className="rounded-md border border-[#37465A] bg-[#18202D] px-2 py-1 text-[#E6E6E6]"
            >
                {OPCIONES.map((o) => (
                    <option key={o.ms} value={o.ms}>
                        {o.texto}
                    </option>
                ))}
            </select>
        </label>
    );
}
