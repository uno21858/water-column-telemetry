-- Esquema de la telemetría de columnas filtrantes
-- Antes de correrlo: exponer "sbagua" en Integrations > Data API > Exposed schemas

create schema if not exists sbagua;

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

-- Un canal de medición por fila. factor y desfase convierten el valor crudo
create table sbagua.sensores (
                                 id          smallint primary key,
                                 columna_id  smallint not null references sbagua.columnas (id),
                                 tipo        text not null check (tipo in ('presion', 'flujo', 'espectro')),
                                 posicion    text check (posicion in ('superior', 'inferior')),
                                 canal_nm    smallint,
                                 modelo      text,
                                 factor      double precision,
                                 desfase     double precision,
                                 unidad      text not null,

-- posición solo para presión, longitud de onda solo para espectro
                                 check ((tipo = 'presion') = (posicion is not null)),
                                 check ((tipo = 'espectro') = (canal_nm is not null))
);

-- Valor crudo tal como lo manda la placa. La hora la pone el servidor
create table sbagua.lecturas (
                                 id         bigint generated always as identity primary key,
                                 ts         timestamptz not null default now(),
                                 sensor_id  smallint not null references sbagua.sensores (id),
                                 valor      integer not null
);

-- Para consultar la serie de un sensor por rango de fechas
create index lecturas_sensor_ts on sbagua.lecturas (sensor_id, ts);

-- Permisos: leer todo, insertar solo lecturas. Sin update ni delete
grant usage on schema sbagua to anon, authenticated;
grant select on all tables in schema sbagua to anon, authenticated;
grant insert on sbagua.lecturas to anon, authenticated;

alter table sbagua.equipos  enable row level security;
alter table sbagua.columnas enable row level security;
alter table sbagua.sensores enable row level security;
alter table sbagua.lecturas enable row level security;

create policy leer on sbagua.equipos  for select to anon using (true);
create policy leer on sbagua.columnas for select to anon using (true);
create policy leer on sbagua.sensores for select to anon using (true);
create policy leer on sbagua.lecturas for select to anon using (true);
create policy insertar on sbagua.lecturas for insert to anon with check (true);