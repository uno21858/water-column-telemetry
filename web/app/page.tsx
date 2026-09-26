import { supabase } from "@/lib/supabase";

export const dynamic = "force-dynamic";

export default async function Page() {
  const { data, error } = await supabase
      .from("lecturas")
      .select("id, ts, sensor_id, valor")
      .order("ts", { ascending: false })
      .limit(20);

  if (error) {
    return <pre>Error: {error.message}</pre>;
  }

  return (
      <main style={{ padding: 32, fontFamily: "monospace" }}>
        <h1>Últimas lecturas</h1>
        <table>
          <thead>
          <tr>
            <th>Hora</th>
            <th>Sensor</th>
            <th>Valor</th>
          </tr>
          </thead>
          <tbody>
          {data.map((l) => (
              <tr key={l.id}>
                <td>
                  {new Date(l.ts).toLocaleString("es-MX", {
                    timeZone: "America/Mexico_City",
                  })}
                </td>
                <td>{l.sensor_id}</td>
                <td>{l.valor}</td>
              </tr>
          ))}
          </tbody>
        </table>
      </main>
  );
}