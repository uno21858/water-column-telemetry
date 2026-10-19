// Hora del laboratorio. México quitó el horario de verano en 2022, así que CDMX es siempre UTC-6
export const ZONA = "America/Mexico_City";
const DESFASE = "-06:00";

// Date -> "2026-10-19T14:30" en hora de CDMX, el formato que usa <input type="datetime-local">
export function aLocal(fecha: Date): string {
    return fecha.toLocaleString("sv-SE", { timeZone: ZONA }).slice(0, 16).replace(" ", "T");
}

// "2026-10-19T14:30" (hora de CDMX) -> Date. null si no tiene ese formato
export function deLocal(texto: string): Date | null {
    if (!/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}$/.test(texto)) {
        return null;
    }
    const fecha = new Date(`${texto}:00${DESFASE}`);
    return Number.isNaN(fecha.getTime()) ? null : fecha;
}

// Date -> "2026-10-19 14:30:00" en hora de CDMX, legible en Excel
export function legible(fecha: Date): string {
    return fecha.toLocaleString("sv-SE", { timeZone: ZONA });
}
