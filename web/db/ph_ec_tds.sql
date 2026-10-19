-- pH, conductividad y TDS para la sala de control (SALA_1)
-- Correr completo en el SQL Editor de Supabase, una sola vez

-- 1. Nuevos tipos de sensor
alter table sbagua.sensores drop constraint sensores_tipo_check;
alter table sbagua.sensores add constraint sensores_tipo_check
    check (tipo in ('presion', 'flujo', 'espectro', 'ph', 'ec', 'tds'));

-- 2. Un sensor de cada tipo en las columnas 1, 2 y 3 del equipo 1.
--    valor es entero, así que la placa manda pH x 100 y el factor lo regresa a pH.
--    EC (uS/cm) y TDS (ppm) llegan tal cual.
--    ids: pH 5-7, EC 8-10, TDS 11-13 (flujo ya usa 1-4)
insert into sbagua.sensores (id, columna_id, tipo, factor, unidad)
select t.base + c.numero, c.id, t.tipo, t.factor, t.unidad
from sbagua.columnas c
         cross join (values (4, 'ph', 100.0, 'pH'),
                            (7, 'ec', 1.0, 'uS/cm'),
                            (10, 'tds', 1.0, 'ppm')) as t(base, tipo, factor, unidad)
where c.equipo_id = 1
  and c.numero between 1 and 3;

-- 3. Cualquier lectura ya convertida a su unidad (flujo queda en L/min)
create view sbagua.medicion with (security_invoker = true) as
select l.ts,
       c.equipo_id,
       c.numero as columna,
       s.tipo,
       l.valor / coalesce(s.factor, 1) as valor
from sbagua.lecturas l
         join sbagua.sensores s on s.id = l.sensor_id
         join sbagua.columnas c on c.id = s.columna_id;

grant select on sbagua.medicion to anon, authenticated;

-- 4. Promedio por tipo y columna en intervalos de "paso", entre dos fechas
create function sbagua.medicion_rango(
    desde  timestamptz,
    hasta  timestamptz,
    paso   interval default '1 minute',
    equipo smallint default 1
)
    returns table (ts timestamptz, tipo text, columna smallint, valor double precision)
    language sql
    stable
as $$
select date_bin(paso, m.ts, desde) as ts,
       m.tipo,
       m.columna,
       avg(m.valor)
from sbagua.medicion m
where m.equipo_id = equipo
  and m.tipo in ('ph', 'ec', 'tds', 'flujo')
  and m.ts >= desde
  and m.ts <  hasta
group by 1, 2, 3
order by 1, 2, 3;
$$;

grant execute on function sbagua.medicion_rango(timestamptz, timestamptz, interval, smallint)
    to anon, authenticated;
