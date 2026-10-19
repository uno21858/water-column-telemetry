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
