import { medicionRangoCompleto, type Tipo } from "@/lib/mediciones";
import { deLocal, legible } from "@/lib/hora";

// Una columna del CSV por sensor
const COLUMNAS: { tipo: Tipo; columna: number; titulo: string }[] = [
    ...[1, 2, 3].map((n) => ({ tipo: "ph" as const, columna: n, titulo: `ph_${n}` })),
    ...[1, 2, 3].map((n) => ({ tipo: "ec" as const, columna: n, titulo: `ec_${n}_uS_cm` })),
    ...[1, 2, 3].map((n) => ({ tipo: "tds" as const, columna: n, titulo: `tds_${n}_ppm` })),
    ...[1, 2].map((n) => ({ tipo: "flujo" as const, columna: n, titulo: `flujo_${n}_L_min` })),
];

// Promedios permitidos, en minutos
const PASOS: Record<string, number> = { "1": 1, "5": 5, "15": 15, "60": 60 };
// Tope de renglones del CSV, para que una sola descarga no tarde minutos (7 días a 1 min)
const MAX_RENGLONES = 7 * 24 * 60;

function error(mensaje: string) {
    return new Response(mensaje, { status: 400, headers: { "Content-Type": "text/plain; charset=utf-8" } });
}

export async function GET(request: Request) {
    const params = new URL(request.url).searchParams;
    const desdeTxt = params.get("desde") ?? "";
    const hastaTxt = params.get("hasta") ?? "";
    const desde = deLocal(desdeTxt);
    const hasta = deLocal(hastaTxt);
    const pasoMin = PASOS[params.get("paso") ?? "1"];

    if (!desde || !hasta) {
        return error("Fechas inválidas. Formato: AAAA-MM-DDTHH:MM");
    }
    if (hasta <= desde) {
        return error("La fecha final tiene que ser después de la inicial");
    }
    if (!pasoMin) {
        return error("Promedio inválido");
    }
    const renglones = (hasta.getTime() - desde.getTime()) / (pasoMin * 60_000);
    if (renglones > MAX_RENGLONES) {
        return error(`Rango muy grande. Con promedio de ${pasoMin} min el máximo es ${Math.floor((MAX_RENGLONES * pasoMin) / 1440)} días`);
    }

    const datos = await medicionRangoCompleto(desde, hasta, `${pasoMin} minutes`);

    // Un renglón por instante, una columna por sensor
    const porTs = new Map<string, Map<string, number>>();
    for (const d of datos) {
        let fila = porTs.get(d.ts);
        if (!fila) {
            fila = new Map();
            porTs.set(d.ts, fila);
        }
        fila.set(`${d.tipo}-${d.columna}`, d.valor);
    }

    const lineas = [["fecha_hora_cdmx", ...COLUMNAS.map((c) => c.titulo)].join(",")];
    for (const [ts, fila] of [...porTs].sort((a, b) => Date.parse(a[0]) - Date.parse(b[0]))) {
        const valores = COLUMNAS.map((c) => {
            const v = fila.get(`${c.tipo}-${c.columna}`);
            return v === undefined ? "" : String(Math.round(v * 1000) / 1000);
        });
        lineas.push([legible(new Date(ts)), ...valores].join(","));
    }

    const nombre = `sala-control_${desdeTxt.replace(":", "")}_a_${hastaTxt.replace(":", "")}.csv`;
    // El BOM hace que Excel abra bien los acentos y la µ
    return new Response("﻿" + lineas.join("\r\n") + "\r\n", {
        headers: {
            "Content-Type": "text/csv; charset=utf-8",
            "Content-Disposition": `attachment; filename="${nombre}"`,
            "Cache-Control": "no-store",
        },
    });
}
