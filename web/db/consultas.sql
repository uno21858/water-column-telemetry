-- Consultas y conversiones sobre las lecturas crudas

-- Provisional: calibración del proyecto anterior. Reemplazar con la propia.
update sbagua.sensores set factor = 313.5 where tipo = 'flujo';

-- Caudal en L/min. valor son pulsos en 60 s, así que pulsos entre pulsos/L da L/min
create view sbagua.caudal with (security_invoker = true) as
select l.ts,
       c.equipo_id,
       c.numero as columna,
       l.valor / s.factor as litros_min
from sbagua.lecturas l
         join sbagua.sensores s on s.id = l.sensor_id
         join sbagua.columnas c on c.id = s.columna_id
where s.tipo = 'flujo';

grant select on sbagua.caudal to anon, authenticated;

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

grant execute on function sbagua.caudal_rango(timestamptz, timestamptz, interval, smallint)
    to anon, authenticated;