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
