import "server-only";
import { supabase } from "@/lib/supabase";

export type Tipo = "ph" | "ec" | "tds" | "flujo";
export type Medicion = { ts: string; tipo: Tipo; columna: number; valor: number };

export async function medicionRango(
    desde: Date,
    hasta: Date,
    paso = "1 minute",
): Promise<Medicion[]> {
    const { data, error } = await supabase.rpc("medicion_rango", {
        desde: desde.toISOString(),
        hasta: hasta.toISOString(),
        paso,
    });

    if (error) {
        throw new Error(`medicion_rango: ${error.message}`);
    }

    return (data ?? []) as Medicion[];
}

// Lectura más reciente de cada sensor, sin importar qué tan vieja sea.
// La placa sube todos los sensores juntos, así que con los últimos renglones alcanza
export async function ultimasLecturas(equipo = 1): Promise<Map<string, { ts: string; valor: number }>> {
    const { data, error } = await supabase
        .from("medicion")
        .select("ts, tipo, columna, valor")
        .eq("equipo_id", equipo)
        .order("ts", { ascending: false })
        .limit(100);

    if (error) {
        throw new Error(`medicion: ${error.message}`);
    }

    const ultimas = new Map<string, { ts: string; valor: number }>();
    for (const d of (data ?? []) as Medicion[]) {
        const llave = `${d.tipo}-${d.columna}`;
        if (!ultimas.has(llave)) {
            ultimas.set(llave, { ts: d.ts, valor: d.valor });
        }
    }
    return ultimas;
}

// Igual que medicionRango, pero trae todo el rango por páginas.
// Supabase corta cada respuesta (1000 filas por defecto), así que se pide hasta que llegue vacía
export async function medicionRangoCompleto(desde: Date, hasta: Date, paso: string): Promise<Medicion[]> {
    const todo: Medicion[] = [];
    const POR_PAGINA = 1000;

    for (;;) {
        const { data, error } = await supabase
            .rpc("medicion_rango", {
                desde: desde.toISOString(),
                hasta: hasta.toISOString(),
                paso,
            })
            .range(todo.length, todo.length + POR_PAGINA - 1);

        if (error) {
            throw new Error(`medicion_rango: ${error.message}`);
        }
        if (!data || data.length === 0) {
            return todo;
        }
        todo.push(...(data as Medicion[]));
    }
}
