"use client";

import { useState } from "react";

const PASOS = [
    { valor: "1", texto: "1 min" },
    { valor: "5", texto: "5 min" },
    { valor: "15", texto: "15 min" },
    { valor: "60", texto: "1 h" },
];

const campo = "rounded-md border border-[#37465A] bg-[#18202D] px-2 py-1 text-[#E6E6E6] [color-scheme:dark]";

// Formulario para bajar las lecturas en CSV. Las fechas van en hora de CDMX
export function Descargar({ desdeInicial, hastaInicial }: { desdeInicial: string; hastaInicial: string }) {
    const [desde, setDesde] = useState(desdeInicial);
    const [hasta, setHasta] = useState(hastaInicial);
    const [paso, setPaso] = useState("1");

    const valido = desde !== "" && hasta !== "" && hasta > desde;
    const url = `/descarga?${new URLSearchParams({ desde, hasta, paso })}`;

    return (
        <div className="flex flex-wrap items-end gap-3 text-sm text-[#969696]">
            <label className="flex flex-col gap-1">
                Desde
                <input type="datetime-local" value={desde} onChange={(e) => setDesde(e.target.value)} className={campo} />
            </label>
            <label className="flex flex-col gap-1">
                Hasta
                <input type="datetime-local" value={hasta} onChange={(e) => setHasta(e.target.value)} className={campo} />
            </label>
            <label className="flex flex-col gap-1">
                Promedio
                <select value={paso} onChange={(e) => setPaso(e.target.value)} className={campo}>
                    {PASOS.map((p) => (
                        <option key={p.valor} value={p.valor}>
                            {p.texto}
                        </option>
                    ))}
                </select>
            </label>
            {valido ? (
                <a
                    href={url}
                    download
                    className="rounded-md bg-[#46A0FF] px-4 py-1.5 font-medium text-[#0D121B] hover:bg-[#6BB4FF]"
                >
                    Descargar CSV
                </a>
            ) : (
                <span className="px-1 py-1.5 text-amber-400">La fecha final tiene que ser después de la inicial</span>
            )}
        </div>
    );
}
