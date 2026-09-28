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

export type SerieCaudal = {
    nombre: string;
    color: string;
    puntos: [number, number | null][];
};

type Props = {
    series: SerieCaudal[];
    desde: number;
    hasta: number;
    minimo: number;
    max: number;
    alto: number;
    zoom?: boolean;
};

export function GraficaCaudal({ series, desde, hasta, minimo, max, alto, zoom = false }: Props) {
    const ref = useRef<HTMLDivElement>(null);

    useEffect(() => {
        const contenedor = ref.current;
        if (!contenedor) {
            return;
        }

        const grafica = echarts.init(contenedor, null, { renderer: "canvas" });

        const opcion: Opcion = {
            backgroundColor: "transparent",
            animation: false,
            grid: { left: 36, right: 12, top: 12, bottom: zoom ? 56 : 24 },
            tooltip: {
                trigger: "axis",
                valueFormatter: (v) => (typeof v === "number" ? `${v.toFixed(2)} L/min` : "sin dato"),
            },
            xAxis: {
                type: "time",
                min: desde,
                max: hasta,
                axisLabel: { color: "#9AA7B4" },
                axisLine: { lineStyle: { color: "#243040" } },
            },
            yAxis: {
                type: "value",
                min: 0,
                max,
                axisLabel: { color: "#9AA7B4" },
                splitLine: { lineStyle: { color: "#243040" } },
            },
            dataZoom: zoom
                ? [
                    { type: "inside", filterMode: "none" },
                    { type: "slider", filterMode: "none", height: 20, bottom: 12 },
                ]
                : [],
            series: series.map((s, i) => ({
                type: "line",
                name: s.nombre,
                data: s.puntos,
                showSymbol: false,
                connectNulls: false,
                lineStyle: { color: s.color, width: 2 },
                itemStyle: { color: s.color },
                markLine:
                    i === 0
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

        grafica.setOption(opcion);

        const observador = new ResizeObserver(() => grafica.resize());
        observador.observe(contenedor);

        return () => {
            observador.disconnect();
            grafica.dispose();
        };
    }, [series, desde, hasta, minimo, max, zoom]);

    return <div ref={ref} style={{ width: "100%", height: alto }} />;
}