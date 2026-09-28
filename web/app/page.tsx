import { caudalRango } from "@/lib/caudal";
import { GraficaCaudal, type SerieCaudal } from "@/components/GraficaCaudal";

export const dynamic = "force-dynamic";

const COLORES = ["#38BDF8", "#FB923C", "#A3E635", "#C084FC"];
const MINIMO = 1.0; // L/min. Abajo de esto el FS300A no arranca
const VENTANA_MS = 40 * 60 * 1000;
const PASO_MS = 60 * 1000;

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

export default async function Page() {
    // Server Component con force-dynamic: se renderiza una vez por request, leer la hora aquí es intencional
    // eslint-disable-next-line react-hooks/purity
    const hasta = Date.now();
    const desde = hasta - VENTANA_MS;
    const puntos = await caudalRango(new Date(desde), new Date(hasta), "1 minute");

    const torres = [1, 2, 3, 4].map((n) => {
        const serie = puntos
            .filter((p) => p.columna === n)
            .map((p) => ({ t: Date.parse(p.ts), v: p.litros_min }));
        const ultimo = serie.length > 0 ? serie[serie.length - 1].v : null;
        const grafica: SerieCaudal = {
            nombre: `Torre ${n}`,
            color: COLORES[n - 1],
            puntos: conHuecos(serie),
        };
        return { n, color: COLORES[n - 1], ultimo, grafica };
    });

    // La escala crece si algún valor pasa de 1.5, redondeando al siguiente 0.5
    const escala = Math.ceil(Math.max(1.5, ...puntos.map((p) => p.litros_min)) * 2) / 2;

    return (
        <main className="min-h-screen bg-[#0F1419] p-4 text-[#E6EDF3] md:p-10">
            <header className="mb-6">
                <h1 className="text-2xl font-semibold">Torres de adsorción</h1>
                <p className="text-sm text-[#9AA7B4]">Caudal por torre, últimos 40 minutos</p>
            </header>

            <div className="flex max-w-[996px] flex-col gap-4">
                <div className="grid grid-cols-1 gap-4 md:grid-cols-2">
                    {torres.map((t) => (
                        <section
                            key={t.n}
                            className="flex flex-col gap-3 rounded-2xl border border-[#243040] bg-[#161D24] p-5"
                        >
                            <div className="flex items-center justify-between gap-2">
                                <h2 className="flex items-center gap-2.5 font-semibold">
                                    <span className="size-2.5 rounded-full" style={{ background: t.color }} />
                                    Torre {t.n}
                                </h2>
                                {t.ultimo !== null && t.ultimo < MINIMO && (
                                    <span className="rounded-full border border-amber-800 px-2 py-0.5 text-xs text-amber-400">
                    Abajo del mínimo del sensor
                  </span>
                                )}
                            </div>

                            <p className="flex items-baseline gap-2">
                                {t.ultimo === null ? (
                                    <span className="text-lg text-[#9AA7B4]">Sin datos en esta ventana</span>
                                ) : (
                                    <>
                                        <span className="font-mono text-3xl">{t.ultimo.toFixed(2)}</span>
                                        <span className="text-sm text-[#9AA7B4]">L/min</span>
                                    </>
                                )}
                            </p>

                            <GraficaCaudal
                                series={[t.grafica]}
                                desde={desde}
                                hasta={hasta}
                                minimo={MINIMO}
                                max={escala}
                                alto={110}
                            />
                        </section>
                    ))}
                </div>

                <section className="flex flex-col gap-3 rounded-2xl border border-[#243040] bg-[#161D24] p-5">
                    <div className="flex flex-wrap items-center justify-between gap-3">
                        <h2 className="font-semibold">Caudal de las 4 torres</h2>
                        <div className="flex flex-wrap gap-4">
                            {torres.map((t) => (
                                <span key={t.n} className="flex items-center gap-2 text-sm text-[#C9D3DD]">
                  <span className="h-[3px] w-4 rounded" style={{ background: t.color }} />
                  Torre {t.n}
                </span>
                            ))}
                        </div>
                    </div>
                    <GraficaCaudal
                        series={torres.map((t) => t.grafica)}
                        desde={desde}
                        hasta={hasta}
                        minimo={MINIMO}
                        max={escala}
                        alto={300}
                        zoom
                    />
                </section>
            </div>
        </main>
    );
}