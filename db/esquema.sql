-- Esquema completo de la telemetría de columnas filtrantes.
--
-- BORRA Y RECREA TODO el esquema sbagua, incluidas las lecturas guardadas.
-- Correr completo en el SQL Editor de Supabase (es una sola transacción).
-- Requisito: "sbagua" expuesto en Integrations > Data API > Exposed schemas.

begin;

drop schema if exists sbagua cascade;
create schema sbagua;

-- ---------------------------------------------------------------------------
-- Tablas
-- ---------------------------------------------------------------------------

-- Montajes físicos (torres de adsorción, columnas de filtración)
create table sbagua.equipos (
    id      smallint primary key,
    nombre  text not null
);

-- Columnas de cada equipo. altura y área son para calcular Darcy
create table sbagua.columnas (
    id         smallint primary key,
    equipo_id  smallint not null references sbagua.equipos (id),
    numero     smallint not null,
    medio      text not null,
    altura_m   numeric,
    area_m2    numeric,
    unique (equipo_id, numero)
);

-- Un canal de medición por fila.
-- columna_id null = sensor conectado pero todavía sin columna asignada.
-- factor: valor / factor da la unidad final. null = el valor ya viene final.
create table sbagua.sensores (
    id          smallint primary key,
    columna_id  smallint references sbagua.columnas (id),
    tipo        text not null,
    posicion    text,
    canal_nm    smallint,
    modelo      text,
    factor      double precision,
    desfase     double precision,
    unidad      text not null,

    constraint tipo_valido
        check (tipo in ('presion', 'flujo', 'espectro', 'ph', 'ec', 'tds')),
    constraint posicion_valida
        check (posicion in ('superior', 'inferior')),
    -- posición solo para presión, longitud de onda solo para espectro
    constraint posicion_solo_presion
        check ((tipo = 'presion') = (posicion is not null)),
    constraint canal_solo_espectro
        check ((tipo = 'espectro') = (canal_nm is not null))
);

-- Valor tal como lo manda la placa. La hora la pone el servidor
create table sbagua.lecturas (
    id         bigint generated always as identity primary key,
    ts         timestamptz not null default now(),
    sensor_id  smallint not null references sbagua.sensores (id),
    valor      double precision not null
);

-- Para consultar la serie de un sensor por rango de fechas
create index lecturas_sensor_ts on sbagua.lecturas (sensor_id, ts);

-- ---------------------------------------------------------------------------
-- Datos fijos
-- ---------------------------------------------------------------------------

insert into sbagua.equipos (id, nombre) values
    (1, 'Columna de intercambio iónico'),  -- Mega + WeMos puente (SALA_1)
    (2, 'Sala 1');

-- AJUSTAR medio del equipo 1
insert into sbagua.columnas (id, equipo_id, numero, medio) values
    (1, 1, 1, 'por definir'),
    (2, 1, 2, 'por definir'),
    (3, 1, 3, 'por definir'),
    (5, 2, 1, 'canicas'),
    (6, 2, 2, 'arena'),
    (7, 2, 3, 'carbón activado');

-- Mega (ARDUINO-SALA-1.ino). Los id deben coincidir con SENSORES[] en
-- src/puente.cpp. Llegan ya calibrados, por eso factor queda null.
-- Las 3 columnas en uso. El flujo 4 (D18) no tiene columna todavía.
insert into sbagua.sensores (id, columna_id, tipo, modelo, unidad) values
    (1,  1,    'flujo', 'YF-S201',        'L/min'),  -- D2
    (2,  2,    'flujo', 'YF-S201',        'L/min'),  -- D3
    (3,  3,    'flujo', 'YF-S201',        'L/min'),  -- D19
    (4,  null, 'flujo', 'YF-S201',        'L/min'),  -- D18
    (5,  1,    'ph',    'pH analógico',   'pH'),     -- A0
    (6,  2,    'ph',    'pH analógico',   'pH'),     -- A1
    (7,  3,    'ph',    'pH analógico',   'pH'),     -- A2
    (8,  1,    'ec',    'TDS Meter V1.0', 'uS/cm'),  -- A4
    (9,  2,    'ec',    'TDS Meter V1.0', 'uS/cm'),  -- A5
    (10, 3,    'ec',    'TDS Meter V1.0', 'uS/cm'),  -- A6
    (11, 1,    'tds',   'TDS Meter V1.0', 'ppm'),    -- A4
    (12, 2,    'tds',   'TDS Meter V1.0', 'ppm'),    -- A5
    (13, 3,    'tds',   'TDS Meter V1.0', 'ppm');    -- A6

-- ---------------------------------------------------------------------------
-- Vistas y funciones
-- ---------------------------------------------------------------------------

-- Caudal en L/min de todos los flujos con columna asignada
create view sbagua.caudal with (security_invoker = true) as
select l.ts,
       c.equipo_id,
       c.numero as columna,
       l.valor / coalesce(s.factor, 1) as litros_min
from sbagua.lecturas l
         join sbagua.sensores s on s.id = l.sensor_id
         join sbagua.columnas c on c.id = s.columna_id
where s.tipo = 'flujo';

-- Cualquier lectura ya convertida a su unidad (la usa el front)
create view sbagua.medicion with (security_invoker = true) as
select l.ts,
       c.equipo_id,
       c.numero as columna,
       s.tipo,
       l.valor / coalesce(s.factor, 1) as valor
from sbagua.lecturas l
         join sbagua.sensores s on s.id = l.sensor_id
         join sbagua.columnas c on c.id = s.columna_id;

-- Promedio por tipo y columna en intervalos de "paso", entre dos fechas
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

-- Caudal promedio por columna en intervalos de "paso", entre dos fechas
create function sbagua.caudal_rango(
    desde  timestamptz,
    hasta  timestamptz,
    paso   interval default '1 minute',
    equipo smallint default 1
)
    returns table (ts timestamptz, columna smallint, litros_min double precision)
    language sql
    stable
as $$
select date_bin(paso, c.ts, desde) as ts,
       c.columna,
       avg(c.litros_min)
from sbagua.caudal c
where c.equipo_id = equipo
  and c.ts >= desde
  and c.ts <  hasta
group by 1, 2
order by 1, 2;
$$;

-- ---------------------------------------------------------------------------
-- Permisos: leer todo, insertar solo lecturas. Sin update ni delete
-- ---------------------------------------------------------------------------

grant usage on schema sbagua to anon, authenticated;
grant select on all tables in schema sbagua to anon, authenticated;
grant insert on sbagua.lecturas to anon, authenticated;
grant execute on function sbagua.caudal_rango(timestamptz, timestamptz, interval, smallint)
    to anon, authenticated;
grant execute on function sbagua.medicion_rango(timestamptz, timestamptz, interval, smallint)
    to anon, authenticated;

alter table sbagua.equipos  enable row level security;
alter table sbagua.columnas enable row level security;
alter table sbagua.sensores enable row level security;
alter table sbagua.lecturas enable row level security;

create policy leer on sbagua.equipos  for select to anon, authenticated using (true);
create policy leer on sbagua.columnas for select to anon, authenticated using (true);
create policy leer on sbagua.sensores for select to anon, authenticated using (true);
create policy leer on sbagua.lecturas for select to anon, authenticated using (true);
create policy insertar on sbagua.lecturas for insert to anon, authenticated with check (true);

commit;
