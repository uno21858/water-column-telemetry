import "server-only";
import { createClient } from "@supabase/supabase-js";

const url = process.env.SUPABASE_URL;
const key = process.env.SUPABASE_KEY;

if (!url || !key) {
    throw new Error("Faltan SUPABASE_URL o SUPABASE_KEY en .env.local");
}

export const supabase = createClient(url, key, {
    db: { schema: "sbagua" },
    auth: { persistSession: false },
});