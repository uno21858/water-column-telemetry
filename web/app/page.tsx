import type { ReactNode } from "react";
import { medicionRango, type Medicion, type Tipo } from "@/lib/mediciones";
import { Grafica, type Serie } from "@/components/Grafica";
import { Refrescar } from "@/components/Refrescar";

export const dynamic = "force-dynamic";

// Colores de SALA_1.pde
const SERIES = ["#46A0FF", "#46D78C", "#F5B446"];
const SENSORES = [1, 2, 3];
const FLUJOS = [1, 2];
const MINIMO_FLUJO = 1.0; // L/min. Abajo de esto el FS300A no arranca
const VENTANA_MS = 40 * 60 * 1000;
const PASO_MS = 60 * 1000;
const EN_LINEA_MS = 3 * 60 * 1000; // la placa sube cada minuto; 3 sin datos = desconectada

// Mete un null donde faltan lecturas, para que la línea se corte en vez de inventar datos
function conHuecos(serie: { t: number; v: number }[]): [number, number | null][] {
    const out: [number, number | null][] = [];
    serie.forEach((p, i) => {
        const anterior = serie[i - 1];
        if (anterior && p.t - anterior.t > PASO_MS * 1.5) {
            out.push([anterior.t + PASO_MS, null]);
        }
        out.push([p.t, p.v]);
    });
    return out;
}

function serieDe(datos: Medicion[], tipo: Tipo, columna: number) {
    return datos
        .filter((d) => d.tipo === tipo && d.columna === columna)
        .map((d) => ({ t: Date.parse(d.ts), v: d.valor }));
}

function ultimo(datos: Medicion[], tipo: Tipo, columna: number): number | null {
    const s = serieDe(datos, tipo, columna);
    return s.length > 0 ? s[s.length - 1].v : null;
}

function series(datos: Medicion[], tipo: Tipo, columnas: number[], nombre: string): Serie[] {
    return columnas.map((n, i) => ({
        nombre: `${nombre} ${n}`,
        color: SERIES[i],
        puntos: conHuecos(serieDe(datos, tipo, n)),
    }));
}

function fmt(v: number | null, decimales: number) {
    return v === null ? "--" : v.toFixed(decimales);
}

function Panel({ titulo, children, className = "" }: { titulo: string; children: ReactNode; className?: string }) {
    return (
        <section className={`flex flex-col gap-4 rounded-xl border border-[#37465A] bg-[#18202D] p-5 ${className}`}>
            <h2 className="text-lg text-[#EBEBEB]">{titulo}</h2>
            {children}
        </section>
    );
}

function Leyenda({ nombres }: { nombres: string[] }) {
    return (
        <div className="flex flex-wrap gap-4">
            {nombres.map((n, i) => (
                <span key={n} className="flex items-center gap-2 text-xs text-[#AAAAAA]">
                    <span className="h-[3px] w-4 rounded" style={{ background: SERIES[i] }} />
                    {n}
                </span>
            ))}
        </div>
    );
}

export default async function Page() {
    // Server Component con force-dynamic: se renderiza una vez por request, leer la hora aquí es intencional
    // eslint-disable-next-line react-hooks/purity
    const hasta = Date.now();
    const desde = hasta - VENTANA_MS;
    const datos = await medicionRango(new Date(desde), new Date(hasta), "1 minute");

    const ultimaTs = datos.length > 0 ? Math.max(...datos.map((d) => Date.parse(d.ts))) : null;
    const enLinea = ultimaTs !== null && hasta - ultimaTs < EN_LINEA_MS;

    const flujos = datos.filter((d) => d.tipo === "flujo" && FLUJOS.includes(d.columna));
    // La escala crece si algún valor pasa de 1.5, redondeando al siguiente 0.5
    const escalaFlujo = Math.ceil(Math.max(1.5, ...flujos.map((d) => d.valor)) * 2) / 2;
    // Como en SALA_1: piso de 2000 uS/cm, crece 10 % arriba del máximo
    const maxEC = Math.max(...datos.filter((d) => d.tipo === "ec").map((d) => d.valor * 1.1), 2000);

    return (
        <main className="min-h-screen bg-[#0D121B] p-4 text-[#F0F0F0] md:p-10">
            <Refrescar cadaMs={PASO_MS} />

            <header className="mb-8 flex flex-wrap items-start justify-between gap-4">
                <div>
                    <h1 className="text-3xl font-semibold tracking-wide">SALA DE CONTROL</h1>
                    <p className="text-sm text-[#969696]">Sistema de adsorción e instrumentación</p>
                </div>
                <div className="flex flex-col gap-1">
                    <span className="flex items-center gap-2.5 text-[15px] text-[#E6E6E6]">
                        <span
                            className="size-[15px] rounded-full"
                            style={{ background: enLinea ? "#46D282" : "#F05A5A" }}
                        />
                        {enLinea ? "RECIBIENDO DATOS" : "SIN DATOS"}
                    </span>
                    <span className="pl-[25px] text-sm text-[#969696]">
                        {ultimaTs === null
                            ? "Nada en los últimos 40 min"
                            : `Última lectura ${new Date(ultimaTs).toLocaleTimeString("es-MX", {
                                  timeZone: "America/Mexico_City",
                              })}`}
                    </span>
                </div>
            </header>

            <div className="grid grid-cols-1 gap-6 lg:grid-cols-[45fr_53fr_38fr]">
                <Panel titulo="pH">
                    <div className="grid grid-cols-3 gap-3">
                        {SENSORES.map((n) => (
                            <div key={n} className="flex flex-col items-center gap-3 rounded-lg bg-[#1F2A3A] px-2 py-5">
                                <span className="text-xs text-[#A0A0A0]">SENSOR {n}</span>
                                <span className="font-mono text-4xl text-[#FFBE41]">{fmt(ultimo(datos, "ph", n), 2)}</span>
                                <span className="text-sm text-[#9B9B9B]">pH</span>
                            </div>
                        ))}
                    </div>
                </Panel>

                <Panel titulo="Conductividad / TDS">
                    <div className="flex flex-col gap-2">
                        {SENSORES.map((n) => (
                            <div
                                key={n}
                                className="grid grid-cols-[auto_1fr_auto] items-center gap-3 rounded-md bg-[#1F2A3A] px-4 py-2.5"
                            >
                                <span className="text-sm text-[#DCDCDC]">SENSOR {n}</span>
                                <span className="text-center font-mono text-lg text-[#41D2DC]">
                                    {fmt(ultimo(datos, "ec", n), 1)} uS/cm
                                </span>
                                <span className="font-mono text-sm text-[#AAAAAA]">
                                    {fmt(ultimo(datos, "tds", n), 1)} ppm
                                </span>
                            </div>
                        ))}
                    </div>
                </Panel>

                <Panel titulo="Caudal">
                    <div className="flex flex-col gap-3">
                        {FLUJOS.map((n) => {
                            const v = ultimo(datos, "flujo", n);
                            return (
                                <div key={n} className="flex flex-col gap-1 rounded-lg bg-[#1F2A3A] px-4 py-3">
                                    <span className="flex items-center justify-between text-xs text-[#A0A0A0]">
                                        FLUJO {n}
                                        {v !== null && v < MINIMO_FLUJO && (
                                            <span className="text-amber-400">Abajo del mínimo del sensor</span>
                                        )}
                                    </span>
                                    <span className="flex items-baseline justify-between">
                                        <span className="font-mono text-3xl text-[#46A0FF]">{fmt(v, 3)}</span>
                                        <span className="text-xs text-[#A0A0A0]">L/min</span>
                                    </span>
                                </div>
                            );
                        })}
                    </div>
                </Panel>
            </div>

            <div className="mt-6 grid grid-cols-1 gap-6 lg:grid-cols-[69fr_70fr]">
                <Panel titulo="Tendencia pH" className="lg:row-span-2">
                    <Leyenda nombres={SENSORES.map((n) => `Sensor ${n}`)} />
                    <Grafica
                        series={series(datos, "ph", SENSORES, "Sensor")}
                        desde={desde}
                        hasta={hasta}
                        unidad="pH"
                        max={14}
                        alto={360}
                        zoom
                    />
                </Panel>

                <Panel titulo="Tendencia conductividad">
                    <Leyenda nombres={SENSORES.map((n) => `Sensor ${n}`)} />
                    <Grafica
                        series={series(datos, "ec", SENSORES, "Sensor")}
                        desde={desde}
                        hasta={hasta}
                        unidad="uS/cm"
                        decimales={1}
                        max={Math.ceil(maxEC)}
                        alto={150}
                    />
                </Panel>

                <Panel titulo="Tendencia caudal">
                    <Leyenda nombres={FLUJOS.map((n) => `Flujo ${n}`)} />
                    <Grafica
                        series={series(datos, "flujo", FLUJOS, "Flujo")}
                        desde={desde}
                        hasta={hasta}
                        unidad="L/min"
                        decimales={3}
                        max={escalaFlujo}
                        minimo={MINIMO_FLUJO}
                        alto={150}
                    />
                </Panel>
            </div>
        </main>
    );
}
