import "server-only";
import { supabase } from "@/lib/supabase";

export type Punto = { ts: string; columna: number; litros_min: number };

export async function caudalRango(
    desde: Date,
    hasta: Date,
    paso = "1 minute",
): Promise<Punto[]> {
    const { data, error } = await supabase.rpc("caudal_rango", {
        desde: desde.toISOString(),
        hasta: hasta.toISOString(),
        paso,
    });

    if (error) {
        throw new Error(`caudal_rango: ${error.message}`);
    }

    return (data ?? []) as Punto[];
}