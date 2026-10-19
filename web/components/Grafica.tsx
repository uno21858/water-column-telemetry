"use client";

import { useEffect, useRef } from "react";
import * as echarts from "echarts/core";
import { LineChart } from "echarts/charts";
import type { LineSeriesOption } from "echarts/charts";
import {
    DataZoomComponent,
    GridComponent,
    MarkLineComponent,
    TooltipComponent,
} from "echarts/components";
import type {
    DataZoomComponentOption,
    GridComponentOption,
    MarkLineComponentOption,
    TooltipComponentOption,
} from "echarts/components";
import { CanvasRenderer } from "echarts/renderers";
import type { ComposeOption } from "echarts/core";

echarts.use([
    LineChart,
    GridComponent,
    TooltipComponent,
    DataZoomComponent,
    MarkLineComponent,
    CanvasRenderer,
]);

type Opcion = ComposeOption<
    | LineSeriesOption
    | GridComponentOption
    | TooltipComponentOption
    | DataZoomComponentOption
    | MarkLineComponentOption
>;

export type Serie = {
    nombre: string;
    color: string;
    puntos: [number, number | null][];
};

type Props = {
    series: Serie[];
    desde: number;
    hasta: number;
    unidad: string;
    decimales?: number;
    min?: number;
    max?: number; // sin max, ECharts ajusta la escala solo
    minimo?: number; // línea punteada de referencia
    alto: number;
    zoom?: boolean;
};

export function Grafica({
    series,
    desde,
    hasta,
    unidad,
    decimales = 2,
    min = 0,
    max,
    minimo,
    alto,
    zoom = false,
}: Props) {
    const ref = useRef<HTMLDivElement>(null);
    const graficaRef = useRef<echarts.ECharts | null>(null);

    // La gráfica se crea una sola vez; con el autorefresh solo cambian los datos
    // y así no parpadea ni pierde el zoom
    useEffect(() => {
        const contenedor = ref.current;
        if (!contenedor) {
            return;
        }

        const grafica = echarts.init(contenedor, null, { renderer: "canvas" });
        graficaRef.current = grafica;

        if (zoom) {
            grafica.setOption({
                dataZoom: [
                    { type: "inside", filterMode: "none" },
                    { type: "slider", filterMode: "none", height: 20, bottom: 12 },
                ],
            });
        }

        const observador = new ResizeObserver(() => grafica.resize());
        observador.observe(contenedor);

        return () => {
            observador.disconnect();
            grafica.dispose();
            graficaRef.current = null;
        };
    }, [zoom]);

    useEffect(() => {
        const grafica = graficaRef.current;
        if (!grafica) {
            return;
        }

        const opcion: Opcion = {
            backgroundColor: "transparent",
            animation: false,
            grid: { left: 44, right: 12, top: 12, bottom: zoom ? 56 : 24 },
            tooltip: {
                trigger: "axis",
                valueFormatter: (v) => (typeof v === "number" ? `${v.toFixed(decimales)} ${unidad}` : "sin dato"),
            },
            xAxis: {
                type: "time",
                min: desde,
                max: hasta,
                axisLabel: { color: "#96A0AA" },
                axisLine: { lineStyle: { color: "#2D3A4B" } },
            },
            yAxis: {
                type: "value",
                min,
                max,
                axisLabel: { color: "#96A0AA" },
                splitLine: { lineStyle: { color: "#2D3A4B" } },
            },
            series: series.map((s, i) => ({
                type: "line",
                name: s.nombre,
                data: s.puntos,
                showSymbol: false,
                connectNulls: false,
                lineStyle: { color: s.color, width: 2 },
                itemStyle: { color: s.color },
                markLine:
                    i === 0 && minimo !== undefined
                        ? {
                            silent: true,
                            symbol: "none",
                            label: { show: false },
                            lineStyle: { color: "#5B6776", type: "dashed" },
                            data: [{ yAxis: minimo }],
                        }
                        : undefined,
            })),
        };

        // replaceMerge: las series se reemplazan, el zoom se queda como estaba
        grafica.setOption(opcion, { replaceMerge: ["series"] });
    }, [series, desde, hasta, unidad, decimales, min, max, minimo, zoom]);

    return <div ref={ref} style={{ width: "100%", height: alto }} />;
}